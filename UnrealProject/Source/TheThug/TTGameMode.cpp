#include "TTGameMode.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"
#include "TTCompanion.h"
#include "TTHUD.h"
#include "TTHospital.h"
#include "TTHunter.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"
#include "TTPlayerController.h"
#include "UnrealClient.h"

ATTGameMode::ATTGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	DefaultPawnClass = ATTPlayerCharacter::StaticClass();
	PlayerControllerClass = ATTPlayerController::StaticClass();
	HUDClass = ATTHUD::StaticClass();
}

void ATTGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bSkipTitle = UGameplayStatics::HasOption(Options, TEXT("skiptitle"));
	Difficulty = static_cast<ETTDifficulty>(FMath::Clamp(UGameplayStatics::GetIntOption(Options, TEXT("diff"), 1), 0, 3));
	int32 CommandLineDifficulty = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("ttdiff="), CommandLineDifficulty))
	{
		Difficulty = static_cast<ETTDifficulty>(FMath::Clamp(CommandLineDifficulty, 0, 3));
	}
	bAutomation = FParse::Param(FCommandLine::Get(), TEXT("ttshots"));
	bDebug = FParse::Param(FCommandLine::Get(), TEXT("ttdebug"));
	bAITest = FParse::Param(FCommandLine::Get(), TEXT("ttai"));
	bLearnTest = FParse::Param(FCommandLine::Get(), TEXT("ttlearn"));
	bSurviveTest = FParse::Param(FCommandLine::Get(), TEXT("ttsurvive"));
	bCompanionTest = FParse::Param(FCommandLine::Get(), TEXT("ttcomp"));
}

void ATTGameMode::EnsureWorld()
{
	if (Hospital)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Hospital = GetWorld()->SpawnActor<ATTHospital>(ATTHospital::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Hospital->Build();

	const FVector HunterLoc = Hospital->CellToWorld(Hospital->GetHunterStartCell(), 102.f);
	Hunter = GetWorld()->SpawnActor<ATTHunter>(ATTHunter::StaticClass(), HunterLoc, FRotator(0.f, 180.f, 0.f), Params);

	// Harley, Bartholomeu and Jack, scattered after Thomas was taken (GDD §3-§4).
	for (int32 i = 0; i < 3; ++i)
	{
		FIntPoint Cell;
		if (!Hospital->GetCompanionStartCell(i, Cell))
		{
			continue;
		}
		if (ATTCompanion* Companion = GetWorld()->SpawnActor<ATTCompanion>(ATTCompanion::StaticClass(), Hospital->CellToWorld(Cell, 94.f),
			FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Params))
		{
			Companion->Setup(static_cast<ETTCompanionId>(i), Difficulty);
			Companions.Add(Companion);
		}
	}
}

void ATTGameMode::RestartPlayer(AController* NewPlayer)
{
	EnsureWorld();
	if (!NewPlayer || !Hospital)
	{
		Super::RestartPlayer(NewPlayer);
		return;
	}
	const FVector Start = Hospital->CellToWorld(Hospital->GetPlayerStartCell(), 96.f);
	const FRotator Rot(0.f, Hospital->GetPlayerStartYaw(), 0.f);
	RestartPlayerAtTransform(NewPlayer, FTransform(Rot, Start));
}

void ATTGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureWorld();

	AmbientWave = TTAudio::MakeStreamingWave(this);
	UpdateAmbience();
	AmbientComponent = UGameplayStatics::SpawnSound2D(this, AmbientWave, 0.55f, 1.f, 0.f, nullptr, false, false);

	SetPhase(ETTGamePhase::Title);
	if (bSkipTitle || bAutomation || bAITest || bLearnTest || bSurviveTest || bCompanionTest)
	{
		StartGame();
	}
}

void ATTGameMode::UpdateAmbience()
{
	if (!AmbientWave)
	{
		return;
	}
	const int32 Target = TTAudio::SampleRate / 2 * static_cast<int32>(sizeof(int16));
	int32 Guard = 0;
	while (AmbientWave->GetAvailableAudioByteCount() < Target && Guard++ < 8)
	{
		TArray<int16> Chunk;
		Drone.Generate(Chunk, TTAudio::SampleRate / 8);
		AmbientWave->QueueAudio(reinterpret_cast<const uint8*>(Chunk.GetData()), Chunk.Num() * sizeof(int16));
	}
}

ATTPlayerCharacter* ATTGameMode::GetPlayer() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? Cast<ATTPlayerCharacter>(PC->GetPawn()) : nullptr;
}

void ATTGameMode::SetPhase(ETTGamePhase NewPhase)
{
	Phase = NewPhase;
	PhaseTime = 0.f;

	ATTPlayerCharacter* Player = GetPlayer();
	const bool bPlaying = Phase == ETTGamePhase::Playing;
	if (Player)
	{
		Player->SetFrozen(!bPlaying);
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetPause(Phase == ETTGamePhase::Paused);
	}
}

void ATTGameMode::StartGame()
{
	if (Phase != ETTGamePhase::Title)
	{
		return;
	}
	SetPhase(ETTGamePhase::Playing);
	RunTime = 0.f;
	if (ATTPlayerCharacter* Player = GetPlayer())
	{
		Player->ApplyLoadout(Difficulty);
	}
	if (Hunter)
	{
		Hunter->Configure(Difficulty);
		Hunter->Activate();
	}
	for (ATTCompanion* Companion : Companions)
	{
		if (Companion)
		{
			Companion->Setup(Companion->GetId(), Difficulty);
			Companion->Activate();
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] Run started. Difficulty=%s"), *TTDifficultyName(Difficulty));
	SetObjective(TEXT("Saia do hospital. Algo está caçando você."));
	ShowMessage(TEXT("Thomas sumiu. Os outros correram. Você está sozinho."), 5.f);
}

void ATTGameMode::TogglePause()
{
	if (Phase == ETTGamePhase::Playing)
	{
		SetPhase(ETTGamePhase::Paused);
	}
	else if (Phase == ETTGamePhase::Paused)
	{
		SetPhase(ETTGamePhase::Playing);
	}
}

void ATTGameMode::RestartRun()
{
	const FString Options = FString::Printf(TEXT("skiptitle?diff=%d"), static_cast<int32>(Difficulty));
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true, Options);
}

void ATTGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void ATTGameMode::PlayerCaptured(ATTHunter* By)
{
	if (Phase != ETTGamePhase::Playing)
	{
		return;
	}
	ATTPlayerCharacter* Player = GetPlayer();
	bCapturedInLocker = Player && Player->IsHidden();
	EndTitle = TEXT("VOCÊ FOI PEGO");
	EndSubtitle = bCapturedInLocker ? TEXT("Ele sabia exatamente onde procurar.") : TEXT("Ele estava mais perto do que parecia.");
	SetPhase(ETTGamePhase::Captured);
	UE_LOG(LogTemp, Log, TEXT("[TT] Player captured after %.1fs (in locker: %d)"), RunTime, bCapturedInLocker ? 1 : 0);
	TTAudio::Play2D(this, ETTSfx::Sting, 1.f);
	if (Player && By)
	{
		Player->LookAtPoint(By->GetEyeLocation());
	}
}

void ATTGameMode::PlayerDied(const FString& Reason)
{
	if (Phase != ETTGamePhase::Playing)
	{
		return;
	}
	EndTitle = TEXT("VOCÊ MORREU");
	EndSubtitle = Reason;
	bCapturedInLocker = false;
	UE_LOG(LogTemp, Log, TEXT("[TT] Joe died after %.1fs: %s"), RunTime, *Reason);
	SetPhase(ETTGamePhase::Captured);
	if (Hunter)
	{
		Hunter->Freeze();
	}
}

void ATTGameMode::RegisterSmoke(ATTSmokeCloud* Smoke)
{
	Smokes.RemoveAll([](const TWeakObjectPtr<ATTSmokeCloud>& S) { return !S.IsValid(); });
	Smokes.Add(Smoke);
}

bool ATTGameMode::IsSightBlockedBySmoke(const FVector& A, const FVector& B) const
{
	for (const TWeakObjectPtr<ATTSmokeCloud>& S : Smokes)
	{
		if (S.IsValid() && S->BlocksSegment(A, B))
		{
			return true;
		}
	}
	return false;
}

void ATTGameMode::OnCompanionTaken(ATTCompanion* Taken)
{
	for (ATTCompanion* Companion : Companions)
	{
		if (Companion && Companion != Taken)
		{
			Companion->OnFriendTaken(Taken);
		}
	}
	if (ATTPlayerCharacter* Joe = GetPlayer())
	{
		if (Taken && FVector::Dist(Joe->GetActorLocation(), Taken->GetActorLocation()) < 3000.f)
		{
			ShowMessage(FString::Printf(TEXT("Um grito no escuro. Era %s."), *Taken->GetDisplayName()), 5.f);
		}
	}
}

void ATTGameMode::PlayerEscaped(const FString& How)
{
	if (Phase != ETTGamePhase::Playing)
	{
		return;
	}
	EndSubtitle = How.IsEmpty() ? TEXT("Pelo portão de serviço. Por enquanto.") : How;
	SetPhase(ETTGamePhase::Escaped);
	if (Hunter)
	{
		Hunter->Freeze();
	}
}

void ATTGameMode::ReportNoise(const FVector& Location, ETTNoise Kind, AActor* NoiseInstigator)
{
	if (Phase != ETTGamePhase::Playing || !Hunter)
	{
		return;
	}
	Hunter->OnNoiseHeard(Location, Kind);
}

void ATTGameMode::OnPlayerLeftLocker(ATTLocker* Locker)
{
	if (Hunter && Phase == ETTGamePhase::Playing)
	{
		Hunter->OnPlayerLeftLocker(Locker);
	}
}

void ATTGameMode::OnPlayerHid(ATTLocker* Locker)
{
	if (Hunter && Phase == ETTGamePhase::Playing)
	{
		Hunter->OnPlayerHid(Locker);
	}
}

void ATTGameMode::ShowMessage(const FString& Text, float Duration)
{
	Message = Text;
	MessageTime = 0.f;
	MessageDuration = Duration;
}

void ATTGameMode::ShowSubtitle(const FString& Line, float Duration)
{
	Subtitle = Line;
	SubtitleTime = 0.f;
	SubtitleDuration = Duration;
}

void ATTGameMode::SetObjective(const FString& Text)
{
	if (Objective != Text)
	{
		Objective = Text;
		ObjectiveTime = 0.f;
	}
}

namespace
{
	float FadeAlpha(float Time, float Duration)
	{
		if (Time >= Duration)
		{
			return 0.f;
		}
		const float In = FMath::Clamp(Time / 0.4f, 0.f, 1.f);
		const float Out = FMath::Clamp((Duration - Time) / 0.8f, 0.f, 1.f);
		return FMath::Min(In, Out);
	}
}

float ATTGameMode::GetMessageAlpha() const { return Message.IsEmpty() ? 0.f : FadeAlpha(MessageTime, MessageDuration); }
float ATTGameMode::GetSubtitleAlpha() const { return Subtitle.IsEmpty() ? 0.f : FadeAlpha(SubtitleTime, SubtitleDuration); }
float ATTGameMode::GetObjectiveAlpha() const { return Objective.IsEmpty() ? 0.f : FadeAlpha(ObjectiveTime, 7.f); }

void ATTGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float RealDelta = FMath::Min(DeltaSeconds, 0.1f);
	PhaseTime += RealDelta;
	if (Phase == ETTGamePhase::Playing)
	{
		RunTime += RealDelta;
		MessageTime += RealDelta;
		SubtitleTime += RealDelta;
		ObjectiveTime += RealDelta;
	}

	UpdateAmbience();
	if (bAutomation)
	{
		UpdateAutomation(RealDelta);
	}
	if (bSurviveTest)
	{
		AutoTime += RealDelta;
		UpdateSurviveTest();
	}
	if (bCompanionTest)
	{
		AutoTime += RealDelta;
		UpdateCompanionTest();
	}
	if (bLearnTest)
	{
		// Pretend he already saw Joe use door (8,4) three times, then watch what he does with it.
		AutoTime += RealDelta;
		if (AutoStep == 0 && AutoTime > 1.f && GetPlayer() && Hospital && Hunter)
		{
			Hunter->DebugObserveRoute(FIntPoint(8, 4), 3);
			Hunter->DebugSetAggression(0.6f);
			ATTLocker* Far = nullptr;
			float FarDist = 0.f;
			for (const TObjectPtr<ATTLocker>& L : Hospital->GetLockers())
			{
				const float D = L ? FVector::Dist2D(L->GetActorLocation(), Hospital->CellToWorld(FIntPoint(8, 4))) : 0.f;
				if (L && D > FarDist)
				{
					FarDist = D;
					Far = L.Get();
				}
			}
			if (Far)
			{
				GetPlayer()->TeleportForTest(Far->GetFrontLocation(), Far->GetFacingYaw() + 180.f);
				GetPlayer()->EnterLocker(Far);
			}
			AutoStep = 1;
		}
		if (AutoTime > 150.f || Phase == ETTGamePhase::Captured)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] Learn test end: phase=%d"), static_cast<int32>(Phase));
			FPlatformMisc::RequestExit(false);
			bLearnTest = false;
		}
	}
	if (bAITest)
	{
		// Joe stands in the corridor with the flashlight on; the Hunter has to find and catch him.
		AutoTime += RealDelta;
		if (AutoStep == 0 && AutoTime > 1.f && GetPlayer() && Hospital)
		{
			GetPlayer()->TeleportForTest(Hospital->CellToWorld(FIntPoint(11, 5)), 90.f);
			AutoStep = 1;
		}
		if ((Phase == ETTGamePhase::Captured && PhaseTime > 2.f) || AutoTime > 150.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] AI test end: phase=%d time=%.1f"), static_cast<int32>(Phase), RunTime);
			FPlatformMisc::RequestExit(false);
			bAITest = false;
		}
	}
}

void ATTGameMode::UpdateCompanionTest()
{
	ATTPlayerCharacter* Joe = GetPlayer();
	if (!Joe || !Hunter || !Hospital || Companions.Num() < 3)
	{
		return;
	}
	ATTCompanion* Harley = Companions[0];
	ATTCompanion* Bart = Companions[1];
	ATTCompanion* Jack = Companions[2];
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(Name) + TEXT(".png"), false, false);
	};
	auto LogAll = [this](const TCHAR* Step)
	{
		for (ATTCompanion* C : Companions)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] Comp %s: %s state=%s %s"), Step, *C->GetDisplayName(), *C->GetStateName(), *C->GetTraitSummary());
		}
	};
	const float Times[] = { 1.f, 1.5f, 3.f, 4.5f, 5.f, 6.f, 7.f, 25.f, 26.f, 60.f, 61.f, 62.f };
	if (AutoStep >= UE_ARRAY_COUNT(Times) || AutoTime < Times[AutoStep])
	{
		return;
	}
	switch (AutoStep)
	{
	case 0:
		Hunter->Freeze();
		LogAll(TEXT("start"));
		break;
	case 1:
	{
		// Joe walks up to Bartholomeu, talks, and gives him a magazine.
		const FVector B = Bart->GetActorLocation();
		Joe->TeleportForTest(B + FVector(-160.f, 0.f, -94.f), 0.f);
		Joe->LookAtPoint(B);
		break;
	}
	case 2:
		Joe->LookAtPoint(Bart->GetActorLocation());
		Joe->Interact();
		Joe->GiveAmmoToFocus();
		Shot(TEXT("TT_10_companion"));
		break;
	case 3:
		LogAll(TEXT("after ammo"));
		break;
	case 4:
		// Harley is out in the lit corridor; the Hunter comes round the corner.
		Harley->DebugForceGoal(1);
		Harley->SetActorLocation(Hospital->CellToWorld(FIntPoint(11, 5), 92.f));
		Hunter->SetActorLocation(Hospital->CellToWorld(FIntPoint(13, 5), 102.f));
		Hunter->SetActorRotation(FRotator(0.f, 180.f, 0.f));
		Hunter->Activate();
		break;
	case 5:
		break;
	case 6:
		// Jack decides to leave through the window.
		Jack->DebugForceGoal(4);
		break;
	case 7:
		LogAll(TEXT("mid"));
		break;
	case 8:
		Shot(TEXT("TT_11_companions"));
		break;
	case 9:
		LogAll(TEXT("late"));
		break;
	case 10:
	{
		// If Jack opened the window, Joe can use it.
		for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] Comp window open: %d"), It->IsOpen() ? 1 : 0);
			if (It->IsOpen() && Phase == ETTGamePhase::Playing)
			{
				Joe->TeleportForTest(It->GetApproachLocation(), (It->GetActorLocation() - It->GetApproachLocation()).Rotation().Yaw);
				It->Interact(Joe);
			}
		}
		break;
	}
	case 11:
		UE_LOG(LogTemp, Log, TEXT("[TT] Comp test end: phase=%d subtitle=%s"), static_cast<int32>(Phase), *EndSubtitle);
		FPlatformMisc::RequestExit(false);
		break;
	}
	++AutoStep;
}

void ATTGameMode::UpdateSurviveTest()
{
	ATTPlayerCharacter* Joe = GetPlayer();
	if (!Joe || !Hunter || !Hospital)
	{
		return;
	}
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(Name) + TEXT(".png"), false, false);
	};
	auto Log = [Joe, this](const TCHAR* Step)
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] Survive %s: injury=%d bleeding=%d blood=%.1f rounds=%d mags=%d grenades=%d smoke=%d bandage=%d slots=%d/%d hunter=%s"),
			Step, Joe->GetInjury(), Joe->IsBleeding() ? 1 : 0, Joe->GetBloodLoss(), Joe->GetRounds(), Joe->GetItemCount(ETTItem::Magazine),
			Joe->GetItemCount(ETTItem::Grenade), Joe->GetItemCount(ETTItem::Smoke), Joe->GetItemCount(ETTItem::Bandage),
			Joe->GetUsedSlots(), Joe->GetCapacity(), *Hunter->GetStateName());
	};
	const FVector Spot = Hospital->CellToWorld(FIntPoint(14, 8));
	const float Times[] = { 1.f, 2.f, 5.f, 5.5f, 6.3f, 6.5f, 7.5f, 7.6f, 10.5f, 11.f, 12.6f, 13.25f, 15.f, 15.5f, 16.5f };
	if (AutoStep >= UE_ARRAY_COUNT(Times) || AutoTime < Times[AutoStep])
	{
		return;
	}
	switch (AutoStep)
	{
	case 0: Log(TEXT("loadout")); Joe->TakeInjury(3, TEXT("Teste.")); Log(TEXT("after severe injury")); break;
	case 1: Joe->TryAddItem(ETTItem::Bandage, 1); Joe->UseMedical(); break;
	case 2: Log(TEXT("after bandage")); Shot(TEXT("TT_05_injured")); break;
	case 3:
	{
		Hunter->Freeze();
		Hunter->SetActorLocation(Hospital->CellToWorld(FIntPoint(11, 8), 102.f));
		Joe->TeleportForTest(Spot, 180.f);
		Joe->LookAtPoint(Hunter->GetEyeLocation());
		Joe->SetAiming(true);
		break;
	}
	case 4:
		Hunter->Activate();
		Joe->LookAtPoint(Hunter->GetActorLocation() + FVector(0.f, 0.f, 20.f));
		Joe->Fire();
		Log(TEXT("after shot"));
		break;
	case 5: Shot(TEXT("TT_06_shot")); break;
	case 6: Joe->SetAiming(false); Hunter->Freeze(); Joe->LookAtPoint(Hunter->GetActorLocation() - FVector(0.f, 0.f, 60.f)); break;
	case 7: Joe->ThrowSmoke(); break;
	case 8:
		Shot(TEXT("TT_07_smoke"));
		UE_LOG(LogTemp, Log, TEXT("[TT] Survive positions: Joe %s Hunter %s"), *Joe->GetActorLocation().ToCompactString(), *Hunter->GetActorLocation().ToCompactString());
		UE_LOG(LogTemp, Log, TEXT("[TT] Survive smoke blocks Hunter->Joe sight: %d"), IsSightBlockedBySmoke(Hunter->GetEyeLocation(), Joe->GetEyeLocation()) ? 1 : 0);
		break;
	case 9: Joe->LookAtPoint(Hunter->GetActorLocation() + FVector(0.f, 0.f, 120.f)); Joe->ThrowGrenade(); break;
	case 10: Hunter->Activate(); break;
	case 11: Shot(TEXT("TT_08_explosion")); break;
	case 12:
		Joe->TeleportForTest(Hospital->CellToWorld(FIntPoint(13, 8)), 180.f);
		Joe->LookAtPoint(Hospital->CellToWorld(FIntPoint(11, 8), 120.f));
		Joe->ThrowSmoke();
		break;
	case 13: Shot(TEXT("TT_09_smoke_close")); break;
	case 14: Log(TEXT("end")); FPlatformMisc::RequestExit(false); break;
	}
	++AutoStep;
}

void ATTGameMode::UpdateAutomation(float DeltaSeconds)
{
	AutoTime += DeltaSeconds;
	ATTPlayerCharacter* Player = GetPlayer();
	if (!Player || !Hospital)
	{
		return;
	}

	auto Shot = [](const TCHAR* Name)
	{
		const FString File = FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(Name) + TEXT(".png");
		FScreenshotRequest::RequestScreenshot(File, false, false);
	};

	struct FStep
	{
		float Time;
		int32 Id;
	};
	static const FStep Steps[] = { { 3.f, 0 }, { 3.5f, 1 }, { 6.f, 2 }, { 6.5f, 3 }, { 9.f, 4 }, { 9.5f, 5 }, { 12.f, 6 }, { 13.f, 7 } };
	if (AutoStep >= UE_ARRAY_COUNT(Steps) || AutoTime < Steps[AutoStep].Time)
	{
		return;
	}

	switch (Steps[AutoStep].Id)
	{
	case 0:
		Shot(TEXT("TT_01_start"));
		break;
	case 1:
		if (Hunter)
		{
			Hunter->Freeze();
		}
		Player->TeleportForTest(Hospital->CellToWorld(FIntPoint(4, 5)), 0.f);
		break;
	case 2:
		Shot(TEXT("TT_02_corridor"));
		break;
	case 3:
		if (Hunter)
		{
			const FVector H = Hunter->GetActorLocation();
			const FVector From = Hospital->CellToWorld(FIntPoint(14, 8));
			Player->TeleportForTest(From, (H - From).Rotation().Yaw);
			Player->LookAtPoint(Hunter->GetEyeLocation());
		}
		break;
	case 4:
		Shot(TEXT("TT_03_hunter"));
		break;
	case 5:
		for (TActorIterator<ATTLocker> It(GetWorld()); It; ++It)
		{
			Player->TeleportForTest(It->GetFrontLocation(), It->GetFacingYaw() + 180.f);
			Player->EnterLocker(*It);
			break;
		}
		break;
	case 6:
		Shot(TEXT("TT_04_locker"));
		break;
	case 7:
		FPlatformMisc::RequestExit(false);
		break;
	}
	++AutoStep;
}
