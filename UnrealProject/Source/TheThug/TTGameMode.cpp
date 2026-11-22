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

THIRD_PARTY_INCLUDES_START
#include "../../../Tools/HunterCore/RunRules.h"
THIRD_PARTY_INCLUDES_END

namespace tr = thug::rules;

/** Tools/HunterCore/RunRules: route prerequisites, failures and the ending. */
struct FTTRunState
{
	tr::RunRules Rules;
	explicit FTTRunState(tr::Difficulty D) : Rules(D) {}
};
#include "TTHospital.h"
#include "TTNarrative.h"
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
	bNoIntro = UGameplayStatics::HasOption(Options, TEXT("nointro"));
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
	bFloorsTest = FParse::Param(FCommandLine::Get(), TEXT("ttfloors"));
	bRouteTest = FParse::Value(FCommandLine::Get(), TEXT("ttroute="), RouteTest);
	FParse::Value(FCommandLine::Get(), TEXT("ttstory="), StoryTest);
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

	Narrative = GetWorld()->SpawnActor<ATTNarrative>(ATTNarrative::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

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
	if (bSkipTitle || bAutomation || bAITest || bLearnTest || bSurviveTest || bCompanionTest || bFloorsTest || bRouteTest || !StoryTest.IsEmpty())
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
	// The opening (N01-N03) plays unless a test runs or the player retries after a defeat.
	const bool bTests = bAutomation || bAITest || bLearnTest || bSurviveTest || bCompanionTest || bFloorsTest || bRouteTest;
	const bool bIntro = !bTests && !bNoIntro && (StoryTest.IsEmpty() || StoryTest == TEXT("intro"));
	if (ATTPlayerCharacter* Player = GetPlayer())
	{
		Player->ApplyLoadout(Difficulty);
	}
	if (Hunter)
	{
		Hunter->Configure(Difficulty);
		if (!bIntro)
		{
			Hunter->Activate();
		}
	}
	for (ATTCompanion* Companion : Companions)
	{
		if (Companion)
		{
			Companion->Setup(Companion->GetId(), Difficulty);
			if (!bIntro)
			{
				Companion->Activate();
			}
		}
	}
	InitRun();
	UE_LOG(LogTemp, Log, TEXT("[TT] Run started. Difficulty=%s intro=%d"), *TTDifficultyName(Difficulty), bIntro ? 1 : 0);
	if (Narrative)
	{
		Narrative->Begin(bIntro ? ATTNarrative::EIntro::Play : (bTests ? ATTNarrative::EIntro::SkipInPlace : ATTNarrative::EIntro::SkipScattered));
	}
	if (!bIntro)
	{
		SetObjective(TEXT("Saia do hospital. Algo está caçando você. [Tab] anotações"));
		ShowMessage(TEXT("Thomas sumiu. Os outros correram. Você está sozinho."), 5.f);
	}
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
	const FString Options = FString::Printf(TEXT("skiptitle?nointro?diff=%d"), static_cast<int32>(Difficulty));
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
	ApplyRunEvent(static_cast<int32>(tr::Event::CapturedByHunter));
	if (Narrative)
	{
		Narrative->OnRunEnded(static_cast<int32>(tr::Ending::Captured), false);
	}
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
	ApplyRunEvent(static_cast<int32>(tr::Event::JoeDied));
	if (Narrative)
	{
		Narrative->OnRunEnded(static_cast<int32>(tr::Ending::Died), false);
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
		const bool bHeard = Taken && FVector::Dist(Joe->GetActorLocation(), Taken->GetActorLocation()) < 3000.f;
		if (bHeard)
		{
			ShowMessage(FString::Printf(TEXT("Um grito no escuro. Era %s."), *Taken->GetDisplayName()), 5.f);
		}
		if (Narrative)
		{
			Narrative->OnCompanionTaken(Taken, bHeard);
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
	if (EndTitle.IsEmpty() || EndTitle.StartsWith(TEXT("VOCÊ FOI")) || EndTitle.StartsWith(TEXT("VOCÊ MORREU")))
	{
		EndTitle = TEXT("VOCÊ ESCAPOU");
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
	/** Cell on a given floor (0 G, 1 B1, 2 F1, 3 F2, 4 F3, 5 F4, 6 roof). */
	FIntPoint FC(int32 Floor, int32 X, int32 Y)
	{
		return FIntPoint(Floor * ATTHospital::Stride + X, Y);
	}

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
	if (Phase == ETTGamePhase::Playing)
	{
		UpdateRun(RealDelta);
	}
	if (bRouteTest)
	{
		AutoTime += RealDelta;
		UpdateRouteTest();
	}
	if (!StoryTest.IsEmpty())
	{
		AutoTime += RealDelta;
		UpdateStoryTest();
	}
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
	if (bFloorsTest)
	{
		AutoTime += RealDelta;
		UpdateFloorsTest();
	}
	if (bLearnTest)
	{
		// Pretend he already saw Joe use door (8,4) three times, then watch what he does with it.
		AutoTime += RealDelta;
		if (AutoStep == 0 && AutoTime > 1.f && GetPlayer() && Hospital && Hunter)
		{
			Hunter->DebugObserveRoute(FC(5, 9, 4), 3);
			Hunter->DebugSetAggression(0.6f);
			ATTLocker* Far = nullptr;
			float FarDist = 0.f;
			for (const TObjectPtr<ATTLocker>& L : Hospital->GetLockers())
			{
				const float D = L ? FVector::Dist2D(L->GetActorLocation(), Hospital->CellToWorld(FC(5, 9, 4))) : 0.f;
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
			GetPlayer()->TeleportForTest(Hospital->CellToWorld(FC(5, 11, 5)), 90.f);
			AutoStep = 1;
		}
		if (FMath::Fmod(AutoTime, 5.f) < RealDelta && Hunter && Hospital && GetPlayer())
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] t=%.0f sense: dist %.0f dz %.0f awareness %.2f joe vis %.2f light %.2f"), AutoTime,
				FVector::Dist(Hunter->GetActorLocation(), GetPlayer()->GetActorLocation()), Hunter->GetActorLocation().Z - GetPlayer()->GetActorLocation().Z,
				Hunter->GetAwareness(), GetPlayer()->GetVisibility(), GetPlayer()->GetLightLevel());
		}
		if (FMath::Fmod(AutoTime, 10.f) < RealDelta && Hunter && Hospital)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] t=%.0f Hunter floor %d (%s) state %s"), AutoTime, Hospital->FloorOfLocation(Hunter->GetActorLocation()),
				*Hospital->GetFloorName(Hospital->FloorOfLocation(Hunter->GetActorLocation())), *Hunter->GetStateName());
		}
		if ((Phase == ETTGamePhase::Captured && PhaseTime > 2.f) || AutoTime > 240.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] AI test end: phase=%d time=%.1f"), static_cast<int32>(Phase), RunTime);
			FPlatformMisc::RequestExit(false);
			bAITest = false;
		}
	}
}

// ---------------------------------------------------------------------------
// Escape routes

void ATTGameMode::InitRun()
{
	static const tr::Difficulty Map[] = { tr::Difficulty::Easy, tr::Difficulty::Normal, tr::Difficulty::Hard, tr::Difficulty::Punitive };
	Run = MakeShared<FTTRunState>(Map[static_cast<int32>(Difficulty)]);
	RunFlags.Reset();
	Notes.Reset();
	// A new combination every night (GDD §53: find it, piece by piece).
	FRandomStream Rng(FMath::Rand());
	SafeCode.Reset();
	for (int32 i = 0; i < 6; ++i)
	{
		SafeCode.AppendInt(i == 0 ? Rng.RandRange(1, 9) : Rng.RandRange(0, 9));
	}
	// Night clock 02:30 -> 06:00. GDD §66: ~30 min (Easy), ~1h30 (Normal), ~3h (Hard). Punitive: no dawn victory.
	static const float RealMinutes[] = { 30.f, 90.f, 180.f, 180.f };
	NightMinutes = 150.f;
	NightRate = 210.f / (RealMinutes[static_cast<int32>(Difficulty)] * 60.f);
	static const float Isolation[] = { 180.f, 300.f, 480.f, 600.f };
	IsolationNeeded = Isolation[static_cast<int32>(Difficulty)];
	float FastHeli = 0.f;
	HeliArrivalDelay = FParse::Value(FCommandLine::Get(), TEXT("ttfastheli="), FastHeli) ? FastHeli : 150.f;
	if (Hospital)
	{
		Hospital->SetPowered(false);
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] Safe code this night: %s"), *SafeCode);
}

bool ATTGameMode::HasRunFlag(const FString& Flag) const
{
	return RunFlags.Contains(Flag);
}

void ATTGameMode::AddNote(const FString& Note)
{
	if (!Notes.Contains(Note))
	{
		Notes.Add(Note);
	}
}

bool ATTGameMode::ApplyRunEvent(int32 Event)
{
	if (!Run)
	{
		return false;
	}
	bool bOk = false;
	try
	{
		bOk = Run->Rules.Apply(static_cast<tr::Event>(Event), GetWorld()->GetTimeSeconds());
	}
	catch (...)
	{
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] RunRules event %d -> %s"), Event, bOk ? TEXT("ok") : TEXT("rejected"));
	return bOk;
}

void ATTGameMode::FinishIfEnded()
{
	if (!Run || Phase != ETTGamePhase::Playing)
	{
		return;
	}
	const tr::Ending Result = Run->Rules.Result();
	if (Result == tr::Ending::None)
	{
		return;
	}
	if (Narrative)
	{
		Narrative->OnRunEnded(static_cast<int32>(Result), true);
	}
	EndTitle = (Result == tr::Ending::SurvivedDawn || Result == tr::Ending::HunterAbandoned || Result == tr::Ending::Destruction)
		? TEXT("VOCÊ SOBREVIVEU") : TEXT("VOCÊ ESCAPOU");
	switch (Result)
	{
	case tr::Ending::Confrontation: PlayerEscaped(TEXT("Confronto. Por alguns segundos, ele ficou no chão.")); break;
	case tr::Ending::Destruction: PlayerEscaped(TEXT("Destruição. O hospital inteiro veio abaixo.")); break;
	case tr::Ending::GarageEscape: PlayerEscaped(TEXT("Pelo portão da garagem, para a rua.")); break;
	case tr::Ending::RoofEscape: PlayerEscaped(TEXT("No helicóptero do chefe. A cidade vai ficando pequena lá embaixo.")); break;
	case tr::Ending::TunnelEscape: PlayerEscaped(TEXT("Pela galeria pluvial, longe do hospital.")); break;
	case tr::Ending::WindowEscape: PlayerEscaped(TEXT("Pela janela, para o prédio vizinho.")); break;
	case tr::Ending::SurvivedDawn: PlayerEscaped(TEXT("O sol nasceu. Ele foi embora. Você continua vivo.")); break;
	case tr::Ending::HunterAbandoned: PlayerEscaped(TEXT("Ele parou de procurar. Por esta noite.")); break;
	default: break;
	}
}

bool ATTGameMode::IsRouteFailed(int32 Route) const
{
	return Run && Route >= 0 && Route <= 3 && Run->Rules.Status(static_cast<tr::Route>(Route)).failed;
}

void ATTGameMode::OnHunterStunned()
{
	if (Narrative && Phase == ETTGamePhase::Playing)
	{
		Narrative->OnHunterStunned();
	}
}

ATTStation* ATTGameMode::FindStationTag(ETTStation Kind, const FString& Tag) const
{
	for (TActorIterator<ATTStation> It(GetWorld()); It; ++It)
	{
		if (It->GetKind() == Kind && (Tag.IsEmpty() || It->GetTag() == Tag))
		{
			return *It;
		}
	}
	return nullptr;
}

ATTStation* ATTGameMode::FindStation(ETTStation Kind) const
{
	for (TActorIterator<ATTStation> It(GetWorld()); It; ++It)
	{
		if (It->GetKind() == Kind)
		{
			return *It;
		}
	}
	return nullptr;
}

FString ATTGameMode::GetStationPrompt(ETTStation Kind) const
{
	switch (Kind)
	{
	case ETTStation::ClueA: return HasRunFlag(TEXT("code_a")) ? TEXT("Bilhete (já lido)") : TEXT("[E] Ler o bilhete no posto de enfermagem");
	case ETTStation::ClueB: return HasRunFlag(TEXT("code_b")) ? TEXT("Pasta (já lida)") : TEXT("[E] Ler a pasta do arquivo");
	case ETTStation::ClueC: return HasRunFlag(TEXT("code_c")) ? TEXT("Etiqueta (já lida)") : TEXT("[E] Ler a etiqueta do quadro elétrico");
	case ETTStation::GuardLog: return TEXT("[E] Ler o livro de ocorrências da guarita");
	case ETTStation::SecurityNote: return TEXT("[E] Ler o memorando da segurança");
	case ETTStation::Safe: return HasRunFlag(TEXT("safe_opened")) ? TEXT("Cofre aberto (vazio)") : TEXT("[E] Teclado do cofre");
	case ETTStation::Generator: return HasRunFlag(TEXT("power_restored")) ? TEXT("Gerador ligado") : TEXT("[E] Dar partida no gerador (muito barulho)");
	case ETTStation::GateControl: return HasRunFlag(TEXT("gate_released")) ? TEXT("Portão liberado") : TEXT("[E] Painel de controle do portão");
	case ETTStation::Radio:
		if (!HasRunFlag(TEXT("radio_found"))) return TEXT("[E] Rádio de comunicação");
		if (!HasRunFlag(TEXT("batteries_installed"))) return TEXT("[E] Rádio (sem pilhas)");
		if (!HasRunFlag(TEXT("radio_contact"))) return TEXT("[E] Tentar contato pelo rádio");
		return TEXT("Rádio (contato feito)");
	case ETTStation::Interference: return HasRunFlag(TEXT("interference_clear")) ? TEXT("Transmissor desligado") : TEXT("[E] Desligar o transmissor (as câmeras caem)");
	case ETTStation::Helipad:
	case ETTStation::TunnelExit:
		return FString();
	}
	return FString();
}

void ATTGameMode::UseStation(ATTStation* Station, ATTPlayerCharacter* Joe)
{
	if (!Station || Phase != ETTGamePhase::Playing)
	{
		return;
	}
	if (Station->GetKind() >= ETTStation::Document)
	{
		if (Narrative)
		{
			Narrative->UseStation(Station, Joe);
		}
		FinishIfEnded();
		return;
	}
	auto Part = [this](int32 Index) { return FString::Printf(TEXT("%c %c"), SafeCode[Index * 2], SafeCode[Index * 2 + 1]); };
	TTAudio::Play2D(this, ETTSfx::Click, 0.35f);
	switch (Station->GetKind())
	{
	case ETTStation::ClueA:
		SetFlag(TEXT("code_a"));
		AddNote(FString::Printf(TEXT("Cofre, parte 1 de 3: %s  (posto de enfermagem, 4º)"), *Part(0)));
		ShowMessage(FString::Printf(TEXT("\"Cofre da adm. Começa com %s. Não deixa o doutor ver.\""), *Part(0)), 6.f);
		break;
	case ETTStation::ClueB:
		SetFlag(TEXT("code_b"));
		AddNote(FString::Printf(TEXT("Cofre, parte 2 de 3: %s  (arquivo, 2º)"), *Part(1)));
		ShowMessage(FString::Printf(TEXT("Na capa da pasta, a lápis: \"...meio: %s\""), *Part(1)), 6.f);
		break;
	case ETTStation::ClueC:
		SetFlag(TEXT("code_c"));
		AddNote(FString::Printf(TEXT("Cofre, parte 3 de 3: %s  (geradores, subsolo)"), *Part(2)));
		ShowMessage(FString::Printf(TEXT("Etiqueta colada no quadro: \"fim: %s\""), *Part(2)), 6.f);
		break;
	case ETTStation::GuardLog:
		SetFlag(TEXT("key_known"));
		AddNote(TEXT("Guarita: a chave reserva do portão da garagem fica num cofre da administração (2º)."));
		ShowMessage(TEXT("\"Chave reserva do portão da garagem: no cofre da administração.\""), 6.f);
		SetObjective(TEXT("A chave do portão da garagem está num cofre no 2º andar."));
		break;
	case ETTStation::SecurityNote:
		SetFlag(TEXT("key_known"));
		SetFlag(TEXT("safe_known"));
		AddNote(TEXT("Segurança: cofre na sala sudeste do 2º. Código em 3 partes: posto de enfermagem (4º), arquivo (2º), geradores (subsolo)."));
		ShowMessage(TEXT("Memorando: o código do cofre foi dividido em três partes, \"por segurança\"."), 6.f);
		SetObjective(TEXT("Junte as três partes do código do cofre (4º, 2º, subsolo)."));
		break;
	case ETTStation::Safe:
		if (HasRunFlag(TEXT("safe_opened")))
		{
			ShowMessage(TEXT("Vazio. Você já pegou a chave."), 2.f);
			break;
		}
		bKeypad = true;
		KeypadEntry.Reset();
		KeypadStation = Station;
		break;
	case ETTStation::Generator:
		if (!HasRunFlag(TEXT("power_restored")))
		{
			SetFlag(TEXT("power_restored"));
			Hospital->SetPowered(true);
			Station->SetStatus(FLinearColor(0.1f, 1.f, 0.2f));
			for (TActorIterator<ATTSign> It(GetWorld()); It; ++It)
			{
				It->SetPoweredLight(true);
			}
			TTAudio::PlayAt(this, ETTSfx::Explosion, Station->GetActorLocation(), 0.5f);
			ReportNoise(Station->GetActorLocation(), ETTNoise::Door, Joe);
			ReportNoise(Station->GetActorLocation(), ETTNoise::Glass, Joe);
			ShowMessage(TEXT("O gerador tosse e ruge. Luzes voltam na garagem. O elevador acende. E todo mundo ouviu."), 6.f);
			AddNote(TEXT("Energia restaurada: garagem e elevador funcionando."));
		}
		break;
	case ETTStation::GateControl:
		if (HasRunFlag(TEXT("gate_released")))
		{
			break;
		}
		if (!HasRunFlag(TEXT("power_restored")))
		{
			ShowMessage(TEXT("Painel morto. Sem energia. Os geradores ficam no subsolo."), 4.f);
			SetObjective(TEXT("Ligue o gerador no subsolo."));
		}
		else if (!HasRunFlag(TEXT("garage_key")))
		{
			ShowMessage(TEXT("\"Liberação manual: insira a chave.\" Você não tem a chave."), 4.f);
		}
		else if (ApplyRunEvent(static_cast<int32>(tr::Event::GarageGateReleased)))
		{
			SetFlag(TEXT("gate_released"));
			Station->SetStatus(FLinearColor(0.1f, 1.f, 0.2f));
			for (TActorIterator<ATTExitGate> It(GetWorld()); It; ++It)
			{
				It->Open();
			}
			ReportNoise(Station->GetActorLocation(), ETTNoise::Door, Joe);
			ShowMessage(TEXT("Um motor geme. O portão da garagem começa a subir. Devagar. Alto."), 5.f);
			SetObjective(TEXT("Saia pelo portão da garagem."));
		}
		break;
	case ETTStation::Radio:
		if (!HasRunFlag(TEXT("radio_found")))
		{
			SetFlag(TEXT("radio_found"));
			ApplyRunEvent(static_cast<int32>(tr::Event::RadioFound));
			AddNote(TEXT("Rádio na emergência (1º): precisa de pilhas. Dá para chamar o chefe."));
		}
		if (!HasRunFlag(TEXT("batteries_installed")))
		{
			if (Joe && Joe->GetItemCount(ETTItem::Batteries) > 0 && Joe->ConsumeItem(ETTItem::Batteries)
				&& ApplyRunEvent(static_cast<int32>(tr::Event::BatteriesInstalled)))
			{
				SetFlag(TEXT("batteries_installed"));
				ShowMessage(TEXT("Pilhas no lugar. O rádio chia, vivo."), 4.f);
			}
			else
			{
				ShowMessage(TEXT("Um rádio de comunicação, sem pilhas. A sala dos servidores deve ter."), 4.f);
				SetObjective(TEXT("Pilhas para o rádio: sala dos servidores, 2º andar."));
			}
			break;
		}
		if (!HasRunFlag(TEXT("interference_clear")))
		{
			TTAudio::Play2D(this, ETTSfx::Hiss, 0.6f);
			ShowMessage(TEXT("Só chiado. Alguma coisa no prédio está interferindo. O transmissor das câmeras?"), 5.f);
			AddNote(TEXT("Rádio com interferência: desligar o transmissor na sala de câmeras (2º)."));
			break;
		}
		if (!HasRunFlag(TEXT("radio_contact")) && ApplyRunEvent(static_cast<int32>(tr::Event::RadioContactEstablished)))
		{
			SetFlag(TEXT("radio_contact"));
			ReportNoise(Station->GetActorLocation(), ETTNoise::Door, Joe);
			if (ApplyRunEvent(static_cast<int32>(tr::Event::HelicopterConfirmed)))
			{
				SetFlag(TEXT("helicopter_confirmed"));
				HeliConfirmedAt = GetWorld()->GetTimeSeconds();
				if (Narrative)
				{
					Narrative->OnRadioContact();
				}
				SetObjective(TEXT("Suba ao telhado. O helicóptero não espera."));
			}
		}
		break;
	case ETTStation::Interference:
		if (!HasRunFlag(TEXT("interference_clear")))
		{
			SetFlag(TEXT("interference_clear"));
			Hospital->BreakAllCameras();
			Station->SetStatus(FLinearColor(0.3f, 0.02f, 0.02f));
			ShowMessage(TEXT("Você desliga o transmissor. Os monitores morrem um a um."), 5.f);
		}
		break;
	default:
		break;
	}
	FinishIfEnded();
}

void ATTGameMode::KeypadDigit(int32 Digit)
{
	if (bKeypad && KeypadEntry.Len() < 6)
	{
		KeypadEntry.AppendInt(Digit);
		TTAudio::Play2D(this, ETTSfx::Click, 0.3f);
	}
}

void ATTGameMode::KeypadBackspace()
{
	if (bKeypad && KeypadEntry.Len() > 0)
	{
		KeypadEntry.LeftChopInline(1);
	}
}

void ATTGameMode::KeypadCancel()
{
	bKeypad = false;
	KeypadEntry.Reset();
}

void ATTGameMode::KeypadSubmit()
{
	if (!bKeypad)
	{
		return;
	}
	ATTStation* Safe = KeypadStation.Get();
	if (KeypadEntry == SafeCode)
	{
		bKeypad = false;
		SetFlag(TEXT("safe_opened"));
		ApplyRunEvent(static_cast<int32>(tr::Event::SafeOpened));
		if (ApplyRunEvent(static_cast<int32>(tr::Event::GarageKeyTaken)))
		{
			SetFlag(TEXT("garage_key"));
			if (ATTPlayerCharacter* Joe = GetPlayer())
			{
				Joe->GiveKey();
			}
		}
		if (Safe)
		{
			Safe->SetStatus(FLinearColor(0.1f, 1.f, 0.2f));
		}
		TTAudio::Play2D(this, ETTSfx::Pickup, 0.7f);
		ShowMessage(TEXT("Clique. A porta pesada abre. A chave do portão da garagem."), 5.f);
		AddNote(TEXT("Chave do portão da garagem: com você. Falta energia (subsolo) e o painel da garagem."));
		SetObjective(HasRunFlag(TEXT("power_restored")) ? TEXT("Libere o portão no painel da garagem.") : TEXT("Ligue o gerador (subsolo) e libere o portão na garagem."));
	}
	else
	{
		TTAudio::Play2D(this, ETTSfx::Click, 0.8f);
		if (Safe)
		{
			ReportNoise(Safe->GetActorLocation(), ETTNoise::QuietDoor, GetPlayer());
		}
		ShowMessage(TEXT("Bip. Código errado."), 2.f);
		KeypadEntry.Reset();
	}
}

void ATTGameMode::UseGarageGate(ATTExitGate* Gate, ATTPlayerCharacter* Joe)
{
	if (Gate && Gate->IsOpen())
	{
		ApplyRunEvent(static_cast<int32>(tr::Event::GarageExitReached));
		FinishIfEnded();
		return;
	}
	TTAudio::Play2D(this, ETTSfx::Click, 0.6f);
	if (!HasRunFlag(TEXT("garage_key")))
	{
		ShowMessage(TEXT("Portão de aço, fechado. Abre pelo painel de controle, com a chave certa."), 4.f);
	}
	else
	{
		ShowMessage(TEXT("Use o painel de controle ao lado para liberar o portão."), 4.f);
	}
}

void ATTGameMode::OnWindowSeen(ATTWindow* Window)
{
	if (!HasRunFlag(TEXT("window_found")) && ApplyRunEvent(static_cast<int32>(tr::Event::WindowFound)))
	{
		SetFlag(TEXT("window_found"));
		AddNote(TEXT("Janela pregada no 3º (leste): dá para o anexo vizinho. Precisa de pé de cabra."));
	}
}

void ATTGameMode::OnWindowOpened(ATTWindow* Window)
{
	// Joe's crowbar or a companion's: the world has validated that the boards came off.
	OnWindowSeen(Window);
	if (ApplyRunEvent(static_cast<int32>(tr::Event::WindowToolsFound)))
	{
		SetFlag(TEXT("window_tools"));
	}
	if (ApplyRunEvent(static_cast<int32>(tr::Event::WindowUnblocked)))
	{
		SetFlag(TEXT("window_unblocked"));
	}
}

void ATTGameMode::UseWindowExit(ATTWindow* Window)
{
	ApplyRunEvent(static_cast<int32>(tr::Event::WindowCrossed));
	FinishIfEnded();
}

void ATTGameMode::DiscoverTunnel(ATTHatch* Grate)
{
	if (HasRunFlag(TEXT("tunnel_found")) || !ApplyRunEvent(static_cast<int32>(tr::Event::TunnelFound)))
	{
		return;
	}
	SetFlag(TEXT("tunnel_found"));
	if (Hospital)
	{
		Hospital->EnableTunnelLinks();
	}
	if (Grate)
	{
		TTAudio::PlayAt(this, ETTSfx::Clank, Grate->GetActorLocation(), 1.f);
		ReportNoise(Grate->GetActorLocation(), ETTNoise::Door, GetPlayer());
	}
	ShowMessage(TEXT("A grade cede. Lá embaixo, água correndo e escuro. Túneis que não estão em mapa nenhum."), 6.f);
	AddNote(TEXT("Túneis sob o subsolo e a garagem. Em algum lugar há uma galeria de descarga para fora."));
}

FString ATTGameMode::GetClockText() const
{
	const int32 Total = FMath::FloorToInt(NightMinutes);
	return FString::Printf(TEXT("%02d:%02d"), (Total / 60) % 24, Total % 60);
}

float ATTGameMode::GetHelicopterSecondsLeft() const
{
	if (!Run || bHeliGone)
	{
		return -1.f;
	}
	const std::optional<double> Deadline = Run->Rules.HelicopterDeadlineSeconds();
	return Deadline ? FMath::Max(0.f, static_cast<float>(*Deadline - GetWorld()->GetTimeSeconds())) : -1.f;
}

bool ATTGameMode::IsHelicopterHere() const
{
	return Helicopter.IsValid() && Helicopter->IsHovering();
}

void ATTGameMode::UpdateRun(float DeltaSeconds)
{
	if (!Run || !Hospital)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	try
	{
		Run->Rules.AdvanceTo(Now);
	}
	catch (...)
	{
	}
	ATTPlayerCharacter* Joe = GetPlayer();

	// Helicopter: arrives after a while, waits on the pad until the deadline.
	if (HeliConfirmedAt >= 0.0 && !bHeliGone)
	{
		if (!Helicopter.IsValid() && Now - HeliConfirmedAt >= HeliArrivalDelay)
		{
			if (ATTStation* Pad = FindStation(ETTStation::Helipad))
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (ATTHelicopter* Heli = GetWorld()->SpawnActor<ATTHelicopter>(ATTHelicopter::StaticClass(), Pad->GetActorLocation(), FRotator::ZeroRotator, Params))
				{
					Heli->Arrive(Pad->GetActorLocation());
					Helicopter = Heli;
					ShowSubtitle(TEXT("CHEFE (rádio): Tô em cima. Sobe, Joe."), 4.f);
					UE_LOG(LogTemp, Log, TEXT("[TT] Helicopter arriving"));
				}
			}
		}
		if (Run->Rules.Status(tr::Route::Roof).failed)
		{
			bHeliGone = true;
			if (Helicopter.IsValid())
			{
				Helicopter->Leave();
			}
			ShowMessage(TEXT("As hélices se afastam. O helicóptero foi embora sem você."), 6.f);
			SetObjective(TEXT("O telhado acabou. Encontre outra saída."));
			UE_LOG(LogTemp, Log, TEXT("[TT] Helicopter left (roof route failed)"));
		}
		else if (Joe && IsHelicopterHere())
		{
			const ATTStation* Pad = FindStation(ETTStation::Helipad);
			if (Pad && FVector::Dist2D(Pad->GetActorLocation(), Joe->GetActorLocation()) < 320.f
				&& Hospital->FloorOfLocation(Joe->GetActorLocation()) == Hospital->FloorOfLocation(Pad->GetActorLocation()))
			{
				ApplyRunEvent(static_cast<int32>(tr::Event::RoofReached));
			}
		}
	}

	// Storm drain exit in the tunnels.
	if (Joe && HasRunFlag(TEXT("tunnel_found")))
	{
		if (const ATTStation* Drain = FindStation(ETTStation::TunnelExit))
		{
			if (FVector::Dist2D(Drain->GetActorLocation(), Joe->GetActorLocation()) < 220.f
				&& Hospital->FloorOfLocation(Joe->GetActorLocation()) == Hospital->FloorOfLocation(Drain->GetActorLocation()))
			{
				ApplyRunEvent(static_cast<int32>(tr::Event::TunnelExitReached));
			}
		}
	}

	// GDD §65: the window can be discovered. Found open by the Hunter, it is nailed shut for good.
	if (Hunter)
	{
		for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It)
		{
			if (It->IsOpen() && !It->IsSealed() && FVector::Dist2D(It->GetApproachLocation(), Hunter->GetActorLocation()) < 350.f
				&& Hospital->FloorOfLocation(Hunter->GetActorLocation()) == Hospital->FloorOfLocation(It->GetApproachLocation()))
			{
				ApplyRunEvent(static_cast<int32>(tr::Event::WindowDiscovered));
				SetFlag(TEXT("window_failed"));
				It->Seal();
				if (Joe && FVector::Dist(Joe->GetActorLocation(), It->GetActorLocation()) < 2500.f)
				{
					ShowMessage(TEXT("Marteladas. Alguém está pregando a janela de volta."), 5.f);
				}
				UE_LOG(LogTemp, Log, TEXT("[TT] Hunter found the open window and sealed it (window route failed)"));
			}
		}
	}

	// Night clock: dawn (not a victory in Punitive).
	NightMinutes += DeltaSeconds * NightRate;
	if (NightMinutes >= 360.f && !bDawnAnnounced)
	{
		bDawnAnnounced = true;
		if (!ApplyRunEvent(static_cast<int32>(tr::Event::DawnReached)))
		{
			ShowMessage(TEXT("Amanheceu. E ele continua aqui."), 6.f);
		}
	}

	// GDD §57: winning by hiding. Only in the isolation room, quiet, unseen.
	if (Joe && Hunter)
	{
		const bool bInIsolation = Hospital->IsIsolationCell(Hospital->WorldToCell(Joe->GetActorLocation()));
		const bool bQuiet = !Joe->IsSprinting() && !Hunter->IsChasing();
		IsolationTime = (bInIsolation && bQuiet) ? IsolationTime + DeltaSeconds : 0.f;
		if (IsolationTime >= IsolationNeeded)
		{
			ApplyRunEvent(static_cast<int32>(tr::Event::HunterAbandonedSearch));
		}
	}

	FinishIfEnded();
}

void ATTGameMode::UpdateStoryTest()
{
	ATTPlayerCharacter* Joe = GetPlayer();
	if (!Joe || !Hunter || !Hospital || !Narrative || Phase == ETTGamePhase::Title)
	{
		return;
	}
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / FString(Name) + TEXT(".png"), false, false);
	};
	auto UseAt = [this, Joe](ATTStation* S)
	{
		if (S)
		{
			Joe->TeleportForTest(S->GetActorLocation() + S->GetActorForwardVector() * 140.f, (-S->GetActorForwardVector()).Rotation().Yaw);
			UseStation(S, Joe);
		}
		UE_LOG(LogTemp, Log, TEXT("[TT] Story test: station %s"), S ? *S->GetTag() : TEXT("MISSING"));
	};
	auto Comp = [this](ETTCompanionId Id) -> ATTCompanion*
	{
		for (ATTCompanion* C : Companions)
		{
			if (C && C->GetId() == Id) return C;
		}
		return nullptr;
	};

	if (Phase != ETTGamePhase::Playing)
	{
		if (PhaseTime > 4.f && AutoStep < 900)
		{
			Shot(*FString::Printf(TEXT("TT_4%d_epilogue_%s"), static_cast<int32>(Difficulty), *StoryTest));
			AutoStep = 900;
		}
		if (PhaseTime > 6.f)
		{
			for (const FTTEpilogueBlock& B : Narrative->GetEpilogue())
			{
				UE_LOG(LogTemp, Log, TEXT("[TT] Epilogue block [%s] %s"), *B.Title, *B.Text);
			}
			UE_LOG(LogTemp, Log, TEXT("[TT] Story test '%s' end: phase=%d title=%s subtitle=%s"), *StoryTest, static_cast<int32>(Phase), *EndTitle, *EndSubtitle);
			FPlatformMisc::RequestExit(false);
			StoryTest.Reset();
		}
		return;
	}
	const float T = AutoTime;
	// Each entry: time, action. AutoStep is the next index.
	auto Due = [this, T](float At) { return T >= At; };

	if (StoryTest == TEXT("intro"))
	{
		switch (AutoStep)
		{
		case 0: if (Due(3.f)) { Shot(TEXT("TT_30_siege")); ++AutoStep; } break;
		case 1: if (Due(24.f)) { Joe->TeleportForTest(Hospital->CellToWorld(FC(5, 21, 7)), 0.f); ++AutoStep; } break;
		case 2: if (Due(27.f)) { Joe->LookAtPoint(Joe->GetActorLocation() + FVector(9600.f, 700.f, 1200.f)); Shot(TEXT("TT_31_signal")); ++AutoStep; } break;
		case 3: if (Due(44.f)) { Joe->TeleportForTest(Hospital->CellToWorld(FC(5, 15, 7)), 180.f); ++AutoStep; } break;
		case 4: if (Narrative->HasSeen(TEXT("N03"))) { ++AutoStep; AutoTime = 100.f; } else if (Due(90.f)) { UE_LOG(LogTemp, Warning, TEXT("[TT] Story test: N03 never played")); ++AutoStep; AutoTime = 100.f; } break;
		case 5: if (Due(100.6f)) { Shot(TEXT("TT_32_rupture")); ++AutoStep; } break;
		case 6: if (Due(115.f)) { UseAt(FindStationTag(ETTStation::Document, TEXT("DOC12"))); ++AutoStep; } break;
		case 7: if (Due(116.f)) { Shot(TEXT("TT_33_document")); ++AutoStep; } break;
		case 8:
			if (Due(118.f))
			{
				Narrative->CloseDocument();
				UE_LOG(LogTemp, Log, TEXT("[TT] Story test: N01=%d N02=%d N03=%d hunter=%s act=%d"), Narrative->HasSeen(TEXT("N01")), Narrative->HasSeen(TEXT("N02")),
					Narrative->HasSeen(TEXT("N03")), *Hunter->GetStateName(), Narrative->GetAct());
				ApplyRunEvent(static_cast<int32>(tr::Event::SafeOpened));
				ApplyRunEvent(static_cast<int32>(tr::Event::GarageKeyTaken));
				ApplyRunEvent(static_cast<int32>(tr::Event::GarageGateReleased));
				ApplyRunEvent(static_cast<int32>(tr::Event::GarageExitReached));
				FinishIfEnded();
				++AutoStep;
			}
			break;
		default: break;
		}
	}
	else if (StoryTest == TEXT("thomas"))
	{
		switch (AutoStep)
		{
		case 0:
			if (Due(2.f))
			{
				Hunter->Freeze();
				UE_LOG(LogTemp, Log, TEXT("[TT] Story test: difficulty %s body=%d tape=%d"), *TTDifficultyName(Difficulty),
					FindStationTag(ETTStation::ThomasBody, FString()) ? 1 : 0, FindStationTag(ETTStation::Document, TEXT("DOC16")) ? 1 : 0);
				UseAt(FindStationTag(ETTStation::ThomasBody, FString()));
				++AutoStep;
			}
			break;
		case 1: if (Due(3.f)) { Shot(TEXT("TT_35_morgue")); ++AutoStep; } break;
		case 2: if (Due(9.f)) { UseAt(FindStationTag(ETTStation::Document, TEXT("DOC16"))); ++AutoStep; } break;
		case 3: if (Due(10.f)) { Shot(TEXT("TT_36_tape")); ++AutoStep; } break;
		case 4: if (Due(12.f)) { Narrative->CloseDocument(); ++AutoStep; } break;
		case 5:
			if (Due(26.f))
			{
				UE_LOG(LogTemp, Log, TEXT("[TT] Story test: N34=%d"), Narrative->HasSeen(TEXT("N34")));
				PlayerCaptured(Hunter);
				++AutoStep;
			}
			break;
		default: break;
		}
	}
	else if (StoryTest == TEXT("jack"))
	{
		ATTCompanion* Jack = Comp(ETTCompanionId::Jack);
		auto TalkToJack = [this, Joe, Jack]()
		{
			Joe->TeleportForTest(Hospital->CellToWorld(FC(4, 5, 5)), 0.f);
			Narrative->DebugTeleportCompanionToJoe(Jack, 170.f);
			Jack->Talk(Joe);
		};
		switch (AutoStep)
		{
		case 0: if (Due(2.f)) { Hunter->Freeze(); TalkToJack(); ++AutoStep; } break;
		case 1: if (Due(12.f)) { Joe->LookAtPoint(Jack->GetActorLocation() + FVector(0.f, 0.f, 50.f)); Shot(TEXT("TT_37_jack_j1")); ++AutoStep; } break;
		case 2: if (Due(24.f)) { SetFlag(TEXT("key_known")); Narrative->DebugSetFact(TEXT("jack_heard_thomas")); ++AutoStep; } break;
		case 3: if (Due(27.f)) { TalkToJack(); ++AutoStep; } break;
		case 4: if (Due(40.f)) { SetFlag(TEXT("safe_known")); SetFlag(TEXT("radio_found")); Narrative->OnCompanionTaken(Comp(ETTCompanionId::Harley), false); ++AutoStep; } break;
		case 5: if (Due(43.f)) { TalkToJack(); ++AutoStep; } break;
		case 6: if (Due(56.f)) { Joe->LookAtPoint(Jack->GetActorLocation() + FVector(0.f, 0.f, 50.f)); Shot(TEXT("TT_38_jack_j3")); SetFlag(TEXT("gate_released")); ++AutoStep; } break;
		case 7: if (Due(72.f)) { TalkToJack(); ++AutoStep; } break;
		case 8:
			if (Due(100.f))
			{
				UE_LOG(LogTemp, Log, TEXT("[TT] Story test: N08=%d J1=%d J2=%d N25=%d J3=%d J4=%d act=%d"), Narrative->HasSeen(TEXT("N08")), Narrative->HasSeen(TEXT("J1")),
					Narrative->HasSeen(TEXT("J2")), Narrative->HasSeen(TEXT("N25")), Narrative->HasSeen(TEXT("J3")), Narrative->HasSeen(TEXT("J4")), Narrative->GetAct());
				PlayerCaptured(Hunter);
				++AutoStep;
			}
			break;
		default: break;
		}
	}
	else if (StoryTest == TEXT("destroy"))
	{
		switch (AutoStep)
		{
		case 0: if (Due(2.f)) { Hunter->Freeze(); UseAt(FindStationTag(ETTStation::Explosives, FString())); ++AutoStep; } break;
		case 1:
			if (Due(4.f))
			{
				for (TActorIterator<ATTStation> It(GetWorld()); It; ++It)
				{
					if (It->GetKind() == ETTStation::Charge)
					{
						UseAt(*It);
					}
				}
				++AutoStep;
			}
			break;
		case 2: if (Due(6.f)) { UseAt(FindStationTag(ETTStation::Detonator, FString())); ++AutoStep; } break;
		default: break;
		}
	}
	else if (StoryTest == TEXT("confront"))
	{
		switch (AutoStep)
		{
		case 0:
			if (Due(2.f))
			{
				Joe->TeleportForTest(Hospital->CellToWorld(FC(5, 11, 7)), 0.f);
				int32 n = 0;
				for (ATTCompanion* C : Companions)
				{
					if (C && n < 2)
					{
						Narrative->DebugTeleportCompanionToJoe(C, 150.f + 120.f * n++);
						C->DebugFollow();
					}
				}
				Hunter->Freeze();
				++AutoStep;
			}
			break;
		case 1: if (Due(7.f)) { Shot(TEXT("TT_39_confront")); ++AutoStep; } break;
		case 2:
			if (Due(8.f))
			{
				Hunter->Activate();
				Hunter->SetActorLocation(Joe->GetActorLocation() + FVector(600.f, 0.f, 10.f));
				Hunter->Stun(4.f, false);
				++AutoStep;
			}
			break;
		case 3: if (Due(20.f)) { UE_LOG(LogTemp, Warning, TEXT("[TT] Story test: no confrontation")); PlayerCaptured(Hunter); ++AutoStep; } break;
		default: break;
		}
	}
	else if (StoryTest == TEXT("art"))
	{
		// Visual tour (M08 art): one screenshot per area.
		struct FView { int32 Floor; int32 X; int32 Y; float Yaw; float Pitch; const TCHAR* Name; };
		static const FView Views[] = {
			{ 4, 13, 4, 0.f, -8.f, TEXT("TT_50_icu") }, { 4, 11, 9, -90.f, -5.f, TEXT("TT_51_clinic") },
			{ 2, 11, 11, -90.f, -5.f, TEXT("TT_52_lobby") }, { 3, 12, 7, 0.f, -5.f, TEXT("TT_53_admin") },
			{ 1, 4, 13, 0.f, 5.f, TEXT("TT_54_b1") }, { 1, 13, 7, 180.f, 5.f, TEXT("TT_55_b1_hall") },
			{ 0, 5, 9, 0.f, 0.f, TEXT("TT_56_garage") }, { 7, 9, 13, 0.f, 5.f, TEXT("TT_57_tunnel") },
			{ 5, 6, 1, 180.f, 10.f, TEXT("TT_58_stairs") }, { 5, 11, 10, 0.f, 0.f, TEXT("TT_59_plate") },
		};
		if (AutoStep == 0 && Due(1.f))
		{
			Hunter->Freeze();
			++AutoStep;
		}
		const int32 Index = (AutoStep - 1) / 2;
		if (AutoStep >= 1 && Index < UE_ARRAY_COUNT(Views) && Due(2.f + AutoStep * 1.5f))
		{
			const FView& V = Views[Index];
			if (AutoStep % 2 == 1)
			{
				Joe->TeleportForTest(Hospital->CellToWorld(FC(V.Floor, V.X, V.Y)), V.Yaw);
				const FVector Fwd = FRotator(V.Pitch, V.Yaw, 0.f).Vector();
				Joe->LookAtPoint(Joe->GetActorLocation() + FVector(0.f, 0.f, 60.f) + Fwd * 1000.f);
			}
			else
			{
				Shot(V.Name);
			}
			++AutoStep;
		}
		else if (Index >= UE_ARRAY_COUNT(Views) && AutoStep < 400 && Due(2.f + AutoStep * 1.5f + 1.f))
		{
			// The cast, lined up in the 4th floor corridor, the Hunter behind them.
			Joe->TeleportForTest(Hospital->CellToWorld(FC(5, 12, 7)), 0.f);
			const FVector Fwd(1.f, 0.f, 0.f);
			const FVector Right(0.f, 1.f, 0.f);
			int32 n = 0;
			for (ATTCompanion* C : Companions)
			{
				if (C)
				{
					Narrative->DebugTeleportCompanionToJoe(C, 320.f);
					C->SetActorLocation(C->GetActorLocation() + Right * (n++ - 1) * 110.f);
					C->HoldForTalk(10.f);
				}
			}
			Hunter->SetActorLocation(Joe->GetActorLocation() + Fwd * 620.f + FVector(0.f, 0.f, 10.f));
			Hunter->SetActorRotation(FRotator(0.f, 180.f, 0.f));
			Joe->LookAtPoint(Joe->GetActorLocation() + Fwd * 1000.f + FVector(0.f, 0.f, 60.f));
			AutoStep = 400;
			AutoTime = 1000.f;
		}
		else if (AutoStep == 400 && Due(1002.f))
		{
			Shot(TEXT("TT_60_cast"));
			AutoStep = 401;
		}
		else if (AutoStep == 401 && Due(1004.f))
		{
			PlayerCaptured(Hunter);
			AutoStep = 500;
		}
	}
	else if (StoryTest == TEXT("robin"))
	{
		switch (AutoStep)
		{
		case 0:
			if (Due(2.f))
			{
				Hunter->Freeze();
				Joe->TeleportForTest(Hospital->CellToWorld(FC(7, 21, 5)), 90.f);
				++AutoStep;
			}
			break;
		case 1:
			if (ATTStoryFigure* R = Narrative->GetRobin())
			{
				if (FVector::Dist(R->GetActorLocation(), Joe->GetActorLocation()) < 1400.f)
				{
					Joe->LookAtPoint(R->GetActorLocation() + FVector(0.f, 0.f, 60.f));
					Shot(TEXT("TT_40_robin"));
					++AutoStep;
				}
			}
			if (Due(70.f)) { ++AutoStep; }
			break;
		case 2:
			if (Narrative->HasFact(TEXT("robin_reported")) || Due(75.f))
			{
				UE_LOG(LogTemp, Log, TEXT("[TT] Story test: robin reported=%d hunter=%s"), Narrative->HasFact(TEXT("robin_reported")), *Hunter->GetStateName());
				PlayerCaptured(Hunter);
				++AutoStep;
			}
			break;
		default: break;
		}
	}
}

void ATTGameMode::UpdateRouteTest()
{
	ATTPlayerCharacter* Joe = GetPlayer();
	if (!Joe || !Hunter || !Hospital || Phase == ETTGamePhase::Title)
	{
		return;
	}
	auto Shot = [](const FString& Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name + TEXT(".png"), false, false);
	};
	auto Goto = [Joe](const AActor* Target, float Back)
	{
		if (Target)
		{
			Joe->TeleportForTest(Target->GetActorLocation() + Target->GetActorForwardVector() * Back - FVector(0.f, 0.f, 0.f),
				(-Target->GetActorForwardVector()).Rotation().Yaw);
		}
	};
	auto Use = [this, Joe, &Goto](ETTStation Kind)
	{
		ATTStation* S = FindStation(Kind);
		Goto(S, 140.f);
		UseStation(S, Joe);
		UE_LOG(LogTemp, Log, TEXT("[TT] Route step %d done (station %s)"), static_cast<int32>(Kind), S ? TEXT("found") : TEXT("MISSING"));
	};
	if (AutoStep == 0 && AutoTime > 1.f)
	{
		Hunter->Freeze();
		++AutoStep;
		return;
	}
	if (Phase != ETTGamePhase::Playing)
	{
		if (PhaseTime > 2.f)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] Route test '%s' end: phase=%d subtitle=%s"), *RouteTest, static_cast<int32>(Phase), *EndSubtitle);
			FPlatformMisc::RequestExit(false);
			bRouteTest = false;
		}
		return;
	}
	const float T = AutoTime;
	if (RouteTest == TEXT("garage"))
	{
		static const float Times[] = { 2.f, 2.5f, 3.f, 3.5f, 4.f, 4.5f, 5.f, 6.f, 7.f, 8.f, 9.f, 14.f, 15.f };
		if (AutoStep - 1 >= UE_ARRAY_COUNT(Times) || T < Times[AutoStep - 1])
		{
			return;
		}
		switch (AutoStep - 1)
		{
		case 0: Use(ETTStation::ClueA); break;
		case 1: Use(ETTStation::ClueB); break;
		case 2: Use(ETTStation::ClueC); break;
		case 3: Use(ETTStation::GuardLog); break;
		case 4: Use(ETTStation::SecurityNote); break;
		case 5: Use(ETTStation::Safe); KeypadEntry = TEXT("000000"); KeypadSubmit(); break;
		case 6: Use(ETTStation::Safe); for (TCHAR Ch : SafeCode) { KeypadDigit(Ch - TEXT('0')); } Shot(TEXT("TT_20_keypad")); KeypadSubmit(); break;
		case 7: Use(ETTStation::GateControl); break;
		case 8: Use(ETTStation::Generator); Shot(TEXT("TT_21_generator")); break;
		case 9: Use(ETTStation::GateControl); break;
		case 10: for (TActorIterator<ATTExitGate> It(GetWorld()); It; ++It) { Goto(*It, 200.f); } Shot(TEXT("TT_22_gate_open")); break;
		case 11: for (TActorIterator<ATTExitGate> It(GetWorld()); It; ++It) { UseGarageGate(*It, Joe); } break;
		default: break;
		}
	}
	else if (RouteTest == TEXT("roof"))
	{
		static const float Times[] = { 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 14.f, 15.f };
		if (AutoStep - 1 >= UE_ARRAY_COUNT(Times) || T < Times[AutoStep - 1])
		{
			return;
		}
		switch (AutoStep - 1)
		{
		case 0: Use(ETTStation::Radio); break;
		case 1: Joe->TryAddItem(ETTItem::Batteries, 1); Use(ETTStation::Radio); break;
		case 2: Use(ETTStation::Radio); break;
		case 3: Use(ETTStation::Interference); break;
		case 4: Use(ETTStation::Radio); break;
		case 5: Goto(FindStation(ETTStation::Helipad), 500.f); break;
		case 6: break;
		case 7: Shot(TEXT("TT_23_helicopter")); break;
		case 8: if (ATTStation* Pad = FindStation(ETTStation::Helipad)) { Joe->TeleportForTest(Pad->GetActorLocation(), 0.f); } break;
		default: break;
		}
	}
	else if (RouteTest == TEXT("tunnel"))
	{
		static const float Times[] = { 2.f, 3.f, 3.5f, 8.f, 9.f, 10.f };
		if (AutoStep - 1 >= UE_ARRAY_COUNT(Times) || T < Times[AutoStep - 1])
		{
			return;
		}
		switch (AutoStep - 1)
		{
		case 0:
			for (TActorIterator<ATTHatch> It(GetWorld()); It; ++It)
			{
				if (It->GetGroup() == TEXT('%') && Hospital->FloorOfLocation(It->GetActorLocation()) == 1)
				{
					Goto(*It, 60.f);
					It->Interact(Joe);
					break;
				}
			}
			break;
		case 1:
			for (TActorIterator<ATTHatch> It(GetWorld()); It; ++It)
			{
				if (It->GetGroup() == TEXT('%') && Hospital->FloorOfLocation(It->GetActorLocation()) == 1)
				{
					It->Interact(Joe);
					break;
				}
			}
			break;
		case 2: break;
		case 3: UE_LOG(LogTemp, Log, TEXT("[TT] Route tunnel: Joe on %s"), *Hospital->GetFloorLabel(Hospital->FloorOfLocation(Joe->GetActorLocation()))); Shot(TEXT("TT_24_tunnels")); break;
		case 4: Goto(FindStation(ETTStation::TunnelExit), 600.f); break;
		case 5: if (ATTStation* Drain = FindStation(ETTStation::TunnelExit)) { Joe->TeleportForTest(Drain->GetActorLocation(), 0.f); } break;
		default: break;
		}
	}
	else if (RouteTest == TEXT("window"))
	{
		static const float Times[] = { 2.f, 3.f, 3.5f, 11.f, 12.f };
		if (AutoStep - 1 >= UE_ARRAY_COUNT(Times) || T < Times[AutoStep - 1])
		{
			return;
		}
		switch (AutoStep - 1)
		{
		case 0:
			Joe->DropItem(ETTItem::Bottle);
			Joe->DropItem(ETTItem::Bottle);
			UE_LOG(LogTemp, Log, TEXT("[TT] Route window: crowbar added %d (slots %d/%d)"), Joe->TryAddItem(ETTItem::Crowbar, 1) ? 1 : 0, Joe->GetUsedSlots(), Joe->GetCapacity());
			break;
		case 1: for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It) { Joe->TeleportForTest(It->GetApproachLocation(), (It->GetActorLocation() - It->GetApproachLocation()).Rotation().Yaw); } break;
		case 2: for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It) { It->Interact(Joe); } break;
		case 3: Shot(TEXT("TT_25_window")); break;
		case 4: for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It) { It->Interact(Joe); } break;
		default: break;
		}
	}
	++AutoStep;
}

void ATTGameMode::UpdateFloorsTest()
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
	auto LogFloor = [Joe, this](const TCHAR* Step)
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] Floors %s: Joe on %s"), Step, *Hospital->GetFloorLabel(Hospital->FloorOfLocation(Joe->GetActorLocation())));
	};
	const float Times[] = { 1.f, 2.f, 3.f, 7.f, 8.f, 9.f, 10.f, 11.5f, 12.f, 13.5f, 14.f, 15.5f, 16.5f };
	if (AutoStep >= UE_ARRAY_COUNT(Times) || AutoTime < Times[AutoStep])
	{
		return;
	}
	switch (AutoStep)
	{
	case 0:
		Hunter->Freeze();
		LogFloor(TEXT("start"));
		// Service hatch on 4 (column 1, row 7): look down and take it.
		Joe->TeleportForTest(Hospital->CellToWorld(FC(5, 1, 7)) + FVector(60.f, 0.f, 0.f), 180.f);
		Joe->LookAtPoint(Hospital->CellToWorld(FC(5, 1, 7), 0.f));
		break;
	case 1:
		UE_LOG(LogTemp, Log, TEXT("[TT] Floors focus: %s"), Joe->GetFocus() ? *Joe->GetFocus()->GetPrompt(Joe) : TEXT("none"));
		Joe->Interact();
		break;
	case 2:
		break;
	case 3:
		LogFloor(TEXT("after crawl"));
		Shot(TEXT("TT_12_after_crawl"));
		break;
	case 4:
		// West stairwell on 3 (even floor): stand on the lower landing, look up the flight.
		Joe->TeleportForTest(Hospital->CellToWorld(FC(4, 2, 4)), -100.f);
		Joe->LookAtPoint(Hospital->CellToWorld(FC(4, 1, 2), 250.f));
		break;
	case 5:
		Shot(TEXT("TT_13_stairs"));
		break;
	case 6:
		// Camera room on 2.
		Joe->TeleportForTest(Hospital->CellToWorld(FC(3, 17, 3)), 0.f);
		Joe->LookAtPoint(Hospital->CellToWorld(FC(3, 18, 3), 100.f));
		Joe->StartCameraView();
		break;
	case 7:
		Shot(TEXT("TT_14_camera"));
		break;
	case 8:
		Joe->CycleCamera(2);
		break;
	case 9:
		Shot(TEXT("TT_15_camera2"));
		break;
	case 10:
		Joe->StopCameraView();
		Joe->TeleportForTest(Hospital->CellToWorld(FC(0, 11, 9)), 90.f);
		break;
	case 11:
		LogFloor(TEXT("garage"));
		Shot(TEXT("TT_16_garage"));
		break;
	case 12:
		UE_LOG(LogTemp, Log, TEXT("[TT] Floors test end"));
		FPlatformMisc::RequestExit(false);
		break;
	}
	++AutoStep;
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
		Harley->SetActorLocation(Hospital->CellToWorld(FC(5, 11, 5), 92.f));
		Hunter->SetActorLocation(Hospital->CellToWorld(FC(5, 11, 9), 102.f));
		Hunter->SetActorRotation(FRotator(0.f, -90.f, 0.f));
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
	const FVector Spot = Hospital->CellToWorld(FC(5, 11, 11));
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
		Hunter->SetActorLocation(Hospital->CellToWorld(FC(5, 11, 8), 102.f));
		Joe->TeleportForTest(Spot, -90.f);
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
		Joe->TeleportForTest(Hospital->CellToWorld(FC(5, 11, 11)), -90.f);
		Joe->LookAtPoint(Hospital->CellToWorld(FC(5, 11, 8), 120.f));
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
		Player->TeleportForTest(Hospital->CellToWorld(FC(5, 5, 7)), 0.f);
		break;
	case 2:
		Shot(TEXT("TT_02_corridor"));
		break;
	case 3:
		if (Hunter)
		{
			const FVector H = Hunter->GetActorLocation();
			const FVector From = Hospital->CellToWorld(FC(6, 9, 5));
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
