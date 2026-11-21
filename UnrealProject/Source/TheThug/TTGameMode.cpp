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
	if (bSkipTitle || bAutomation || bAITest || bLearnTest)
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
		static const int32 Bottles[] = { 3, 2, 1, 0 };
		Player->AddBottles(Bottles[static_cast<int32>(Difficulty)] - Player->GetBottles());
	}
	if (Hunter)
	{
		Hunter->Configure(Difficulty);
		Hunter->Activate();
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
	SetPhase(ETTGamePhase::Captured);
	UE_LOG(LogTemp, Log, TEXT("[TT] Player captured after %.1fs (in locker: %d)"), RunTime, bCapturedInLocker ? 1 : 0);
	TTAudio::Play2D(this, ETTSfx::Sting, 1.f);
	if (Player && By)
	{
		Player->LookAtPoint(By->GetEyeLocation());
	}
}

void ATTGameMode::PlayerEscaped()
{
	if (Phase != ETTGamePhase::Playing)
	{
		return;
	}
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
