#include "TTHunter.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TTAudio.h"
#include "TTCompanion.h"
#include "EngineUtils.h"
#include "TTGameMode.h"
#include "TTHospital.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"
#include "TTShapes.h"

THIRD_PARTY_INCLUDES_START
#include "../../../Tools/HunterCore/HunterCore.h"
#include "../../../Tools/PerceptionCore/PerceptionCore.h"
THIRD_PARTY_INCLUDES_END

namespace th = thug::hunter;
namespace tp = thug::perception;

/** The engine-independent brain, ears and evidence memory from /Tools. */
struct FTTHunterMind
{
	th::HunterCore Brain;
	tp::PerceptionCore Ears;
	tp::EvidenceMemory Eyes;
	th::Tuning Tuning;

	FTTHunterMind(th::Difficulty InDifficulty, uint64 Seed)
		: Brain(InDifficulty, Seed)
		, Ears(tp::Tuning{}, Seed ^ 0x9E3779B97F4A7C15ull)
		, Tuning(th::TuningFor(InDifficulty))
	{
	}
};

namespace HunterTuning
{
	constexpr float PatrolSpeed = 175.f;
	constexpr float SearchSpeed = 215.f;
	constexpr float InvestigateSpeed = 280.f;
	constexpr float PursueSpeed = 445.f;
	constexpr float SightRange = 2400.f;
	constexpr float PursueSightRange = 3200.f;
	constexpr float CloseSense = 230.f;
	constexpr float CaptureDistance = 145.f;
	constexpr float FaintSoundStrength = 0.12f;
}

namespace
{
	th::Vec3 ToCore(const FVector& V)
	{
		return th::Vec3{ V.X / 100.0, V.Y / 100.0, V.Z / 100.0 };
	}

	FVector FromCore(const th::Vec3& V)
	{
		return FVector(V.x * 100.0, V.y * 100.0, V.z * 100.0);
	}

	th::Difficulty ToCore(ETTDifficulty D)
	{
		switch (D)
		{
		case ETTDifficulty::Easy: return th::Difficulty::Easy;
		case ETTDifficulty::Hard: return th::Difficulty::Hard;
		case ETTDifficulty::Punitive: return th::Difficulty::Punitive;
		default: return th::Difficulty::Normal;
		}
	}

	void ToCore(ETTNoise Kind, tp::SoundKind& OutKind, double& OutEnergy)
	{
		OutEnergy = 1.0;
		switch (Kind)
		{
		case ETTNoise::SlowStep: OutKind = tp::SoundKind::SlowFootstep; break;
		case ETTNoise::Step: OutKind = tp::SoundKind::WalkFootstep; break;
		case ETTNoise::SprintStep: OutKind = tp::SoundKind::SprintFootstep; break;
		case ETTNoise::Door: OutKind = tp::SoundKind::Door; break;
		case ETTNoise::QuietDoor: OutKind = tp::SoundKind::Door; OutEnergy = 0.3; break;
		case ETTNoise::Locker: OutKind = tp::SoundKind::Door; OutEnergy = 0.35; break;
		case ETTNoise::Glass: OutKind = tp::SoundKind::BrokenObject; break;
		case ETTNoise::Breath: OutKind = tp::SoundKind::Breathing; break;
		case ETTNoise::GlassCrunch: OutKind = tp::SoundKind::BrokenObject; OutEnergy = 0.4; break;
		case ETTNoise::Gunshot: OutKind = tp::SoundKind::Gunshot; break;
		case ETTNoise::Explosion: OutKind = tp::SoundKind::Explosion; break;
		}
	}

	std::string ToStd(const FString& S)
	{
		return std::string(TCHAR_TO_UTF8(*S));
	}

	const TCHAR* StateName(ETTHunterState S)
	{
		switch (S)
		{
		case ETTHunterState::Dormant: return TEXT("Dormant");
		case ETTHunterState::Patrol: return TEXT("Patrol");
		case ETTHunterState::LookAround: return TEXT("LookAround");
		case ETTHunterState::Suspicious: return TEXT("Suspicious");
		case ETTHunterState::Investigate: return TEXT("Investigate");
		case ETTHunterState::Search: return TEXT("Search");
		case ETTHunterState::Pursue: return TEXT("Pursue");
		case ETTHunterState::CheckLocker: return TEXT("CheckLocker");
		case ETTHunterState::Ambush: return TEXT("Ambush");
		case ETTHunterState::SetTrap: return TEXT("SetTrap");
		case ETTHunterState::Stunned: return TEXT("Stunned");
		case ETTHunterState::PursueCompanion: return TEXT("PursueCompanion");
		case ETTHunterState::Interrogate: return TEXT("Interrogate");
		case ETTHunterState::Capturing: return TEXT("Capturing");
		}
		return TEXT("?");
	}

	bool IsDirectPathClear(const UWorld* World, const AActor* Self, const AActor* Ignore, const FVector& From, const FVector& To)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(TTHunterPath), false, Self);
		if (Ignore)
		{
			Query.AddIgnoredActor(Ignore);
		}
		const FVector B(To.X, To.Y, From.Z);
		return !World->SweepTestByChannel(From, B, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(38.f, 60.f), Query);
	}

	const FIntPoint Around[8] = {
		FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1),
		FIntPoint(1, 1), FIntPoint(-1, 1), FIntPoint(1, -1), FIntPoint(-1, -1)
	};

	bool BrainObserve(FTTHunterMind& Mind, const th::Stimulus& Stimulus)
	{
		try
		{
			return Mind.Brain.Observe(Stimulus);
		}
		catch (...)
		{
			return false;
		}
	}
}

// ---------------------------------------------------------------------------
// Setup

ATTHunter::ATTHunter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 100.f);

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 400.f, 0.f);
	Move->MaxWalkSpeed = HunterTuning::PatrolSpeed;
	Move->MaxAcceleration = 1200.f;
	Move->BrakingDecelerationWalking = 1400.f;
}

ATTHunter::~ATTHunter() = default;

void ATTHunter::BeginPlay()
{
	Super::BeginPlay();
	BuildVisual();
	if (!Mind)
	{
		Configure(Difficulty);
	}
}

void ATTHunter::Configure(ETTDifficulty InDifficulty)
{
	Difficulty = InDifficulty;
	Mind = MakeShared<FTTHunterMind>(ToCore(Difficulty), static_cast<uint64>(FMath::Rand()) + 1ull);

	static const float SpeedScales[] = { 0.88f, 0.95f, 1.f, 1.06f };
	static const float SightScales[] = { 0.8f, 0.9f, 1.f, 1.1f };
	static const int32 Targets[] = { 3, 4, 5, 7 };
	const int32 Index = static_cast<int32>(Difficulty);
	SpeedScale = SpeedScales[Index];
	SightScale = SightScales[Index];
	SearchTargets = Targets[Index];
	SearchSeconds = static_cast<float>(Mind->Tuning.searchSeconds);
}

void ATTHunter::BuildVisual()
{
	USceneComponent* Root = GetCapsuleComponent();
	const FLinearColor Black(0.004f, 0.004f, 0.005f);
	const FLinearColor Suit(0.018f, 0.018f, 0.022f);

	// 2 m tall: the Mannequin (180 cm) scaled up, charcoal suit, darker cowl and boots.
	const bool bBody = TTShapes::SetupMannequin(this, false, FLinearColor(0.008f, 0.008f, 0.01f), Suit, 1.11f, 0.5f) != nullptr;
	if (!bBody)
	{
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, -13.f, -52.f), FVector(22.f, 22.f, 96.f), Suit, FRotator::ZeroRotator, false);
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 13.f, -52.f), FVector(22.f, 22.f, 96.f), Suit, FRotator::ZeroRotator, false);
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 32.f), FVector(40.f, 62.f, 82.f), Suit, FRotator::ZeroRotator, false);
		TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(2.f, 0.f, 88.f), FVector(30.f, 28.f, 32.f), Black, FRotator::ZeroRotator, false);
	}
	// Cape from the shoulders; it swings back with speed (see Tick).
	CapePivot = NewObject<USceneComponent>(this, TEXT("CapePivot"));
	CapePivot->SetupAttachment(Root);
	CapePivot->SetRelativeLocation(FVector(-14.f, 0.f, 64.f));
	CapePivot->RegisterComponent();
	FTTSurface CapeCloth = TTShapes::Worn(FLinearColor(0.006f, 0.006f, 0.007f), 0.97f, 0.15f);
	for (int32 Panel = -1; Panel <= 1; ++Panel)
	{
		// Three panels fanning out a little: reads as cloth, not a board.
		if (UStaticMeshComponent* Cloth = TTShapes::AddPart(this, CapePivot, TTShapes::Cube(), FVector(-4.f - FMath::Abs(Panel) * 3.f, Panel * 19.f, -76.f), FVector(3.f, 24.f, 150.f), Black, FRotator(0.f, Panel * 10.f, Panel * 4.f), false))
		{
			Cloth->SetMaterial(0, TTShapes::MakeSurface(Cloth, CapeCloth));
		}
	}
	if (UStaticMeshComponent* Collar = TTShapes::AddPart(this, CapePivot, TTShapes::Cube(), FVector(-2.f, 0.f, -2.f), FVector(14.f, 50.f, 7.f), Black, FRotator::ZeroRotator, false))
	{
		Collar->SetMaterial(0, TTShapes::MakeSurface(Collar, CapeCloth));
	}
	// Cowl ears, belt.
	TTShapes::AddPart(this, Root, TTShapes::Cone(), FVector(-1.f, -6.5f, 104.f), FVector(6.5f, 6.5f, 17.f), Black, FRotator(0.f, 0.f, -6.f), false);
	TTShapes::AddPart(this, Root, TTShapes::Cone(), FVector(-1.f, 6.5f, 104.f), FVector(6.5f, 6.5f, 17.f), Black, FRotator(0.f, 0.f, 6.f), false);
	TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(1.f, 0.f, 8.f), FVector(30.f, 38.f, 7.f), FLinearColor(0.32f, 0.26f, 0.06f), FRotator::ZeroRotator, false);
	for (const float Side : { -4.f, 4.f })
	{
		if (UStaticMeshComponent* Eye = TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(11.f, Side, 85.f), FVector(3.f, 4.f, 1.6f), FLinearColor(0.8f, 0.85f, 0.9f), FRotator::ZeroRotator, false))
		{
			Eye->SetMaterial(0, TTShapes::MakeEmissive(Eye, FLinearColor(0.75f, 0.85f, 1.f), 3.f));
		}
	}

	UPointLightComponent* EyeGlow = NewObject<UPointLightComponent>(this);
	EyeGlow->SetupAttachment(Root);
	EyeGlow->SetRelativeLocation(FVector(20.f, 0.f, 85.f));
	EyeGlow->SetIntensityUnits(ELightUnits::Lumens);
	EyeGlow->SetIntensity(2.5f);
	EyeGlow->SetAttenuationRadius(60.f);
	EyeGlow->SetLightColor(FLinearColor(0.75f, 0.85f, 1.f));
	EyeGlow->SetCastShadows(false);
	EyeGlow->RegisterComponent();
}

ATTHospital* ATTHunter::GetHospital() const
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	return GM ? GM->GetHospital() : nullptr;
}

ATTPlayerCharacter* ATTHunter::GetPlayer() const
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	return GM ? GM->GetPlayer() : nullptr;
}

double ATTHunter::Now() const
{
	return GetWorld()->GetTimeSeconds();
}

float ATTHunter::Speed(float Base) const
{
	return Base * SpeedScale * (1.f + 0.12f * Aggression);
}

FString ATTHunter::GetStateName() const
{
	return StateName(State);
}

FString ATTHunter::GetMemorySummary() const
{
	const ATTHospital* Hospital = GetHospital();
	if (!Hospital || !Mind)
	{
		return FString();
	}
	FString Best;
	float BestScore = 0.f;
	FString Locked;
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Hospital->GetDoors())
	{
		const float Score = RouteEvidence(Pair.Key);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = ATTHospital::DoorId(Pair.Key);
		}
		if (Pair.Value && Pair.Value->IsLocked())
		{
			Locked = ATTHospital::DoorId(Pair.Key);
		}
	}
	FString Out = Best.IsEmpty() ? TEXT("rota: -") : FString::Printf(TEXT("rota: %s %.1f"), *Best, BestScore);
	if (!Locked.IsEmpty())
	{
		Out += FString::Printf(TEXT("  travada: %s"), *Locked);
	}
	return Out;
}

void ATTHunter::Activate()
{
	if (State == ETTHunterState::Dormant)
	{
		StartPatrol();
	}
}

void ATTHunter::Freeze()
{
	SetState(ETTHunterState::Dormant);
	Path.Reset();
	GetCharacterMovement()->StopMovementImmediately();
}

void ATTHunter::SetState(ETTHunterState NewState)
{
	if (State != NewState)
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] Hunter %s -> %s"), StateName(State), StateName(NewState));
	}
	if (State == ETTHunterState::PursueCompanion && NewState != ETTHunterState::PursueCompanion
		&& NewState != ETTHunterState::Interrogate && NewState != ETTHunterState::CheckLocker)
	{
		if (ATTCompanion* Companion = TargetCompanion.Get())
		{
			Companion->OnChasedByHunter(false);
		}
	}
	PreviousState = State;
	State = NewState;
	StateTime = 0.f;
	if (NewState == ETTHunterState::LookAround)
	{
		LookBaseYaw = GetActorRotation().Yaw;
	}
}

void ATTHunter::Say(const FString& Line, float Chance)
{
	if (SpeechCooldown > 0.f || FMath::FRand() > Chance)
	{
		return;
	}
	SpeechCooldown = 9.f;
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->ShowSubtitle(Line, 3.5f);
	}
}

// ---------------------------------------------------------------------------
// Memory (HunterCore)

float ATTHunter::HideSpotSuspicion(const ATTLocker* Locker) const
{
	return (Mind && Locker) ? static_cast<float>(Mind->Brain.HideSpotSuspicion(ToStd(Locker->GetId()))) : 0.f;
}

float ATTHunter::RouteEvidence(const FIntPoint& DoorCell) const
{
	return Mind ? static_cast<float>(Mind->Brain.RouteEvidence(ToStd(ATTHospital::DoorId(DoorCell)))) : 0.f;
}

void ATTHunter::ObserveHideSpot(const ATTLocker* Locker, double Strength)
{
	if (!Mind || !Locker)
	{
		return;
	}
	const th::Stimulus S(th::StimulusKind::ConfirmedHideSpot, th::Subject::Player, Now(), std::nullopt, Strength, ToStd(Locker->GetId()));
	if (BrainObserve(*Mind, S))
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] Memory: saw Joe use %s -> suspicion %.0f"), *Locker->GetId(), HideSpotSuspicion(Locker));
	}
}

void ATTHunter::DebugObserveRoute(const FIntPoint& DoorCell, int32 Times)
{
	if (!Mind)
	{
		return;
	}
	for (int32 i = 0; i < Times; ++i)
	{
		BrainObserve(*Mind, th::Stimulus(th::StimulusKind::ObservedRoute, th::Subject::Player, Now(), std::nullopt, 1.0, ToStd(ATTHospital::DoorId(DoorCell))));
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] Debug: route %s evidence %.2f"), *ATTHospital::DoorId(DoorCell), RouteEvidence(DoorCell));
}

// ---------------------------------------------------------------------------
// Perception

bool ATTHunter::CanSeePoint(const FVector& Point, const AActor* Target) const
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTHunterLook), false, this);
	if (const ATTPlayerCharacter* Player = GetPlayer())
	{
		Query.AddIgnoredActor(Player);
	}
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, GetEyeLocation(), Point, ECC_Visibility, Query))
	{
		return true;
	}
	return Target && Hit.GetActor() == Target;
}

bool ATTHunter::CanSeePlayer(float& OutDistance, float& OutVisibility) const
{
	const ATTPlayerCharacter* Player = GetPlayer();
	if (!Player || Player->IsHidden())
	{
		return false;
	}

	const FVector Eye = GetEyeLocation();
	const FVector Target = Player->GetEyeLocation();
	const FVector ToPlayer = Target - Eye;
	OutDistance = ToPlayer.Size();
	OutVisibility = Player->GetVisibility();

	const bool bChasing = State == ETTHunterState::Pursue;
	if (OutDistance > HunterTuning::CloseSense)
	{
		const float CosHalfAngle = bChasing ? 0.08f : 0.5f;
		if (FVector::DotProduct(GetActorForwardVector(), ToPlayer.GetSafeNormal()) < CosHalfAngle)
		{
			return false;
		}
		const float Range = (bChasing ? HunterTuning::PursueSightRange : HunterTuning::SightRange * OutVisibility) * SightScale;
		if (OutDistance > Range)
		{
			return false;
		}
	}

	if (OutDistance > 120.f)
	{
		if (const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
		{
			if (GM->IsSightBlockedBySmoke(Eye, Target) && GM->IsSightBlockedBySmoke(Eye, Player->GetActorLocation()))
			{
				return false;
			}
		}
	}

	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTHunterSight), false, this);
	Query.AddIgnoredActor(Player);
	if (!GetWorld()->LineTraceTestByChannel(Eye, Target, ECC_Visibility, Query))
	{
		return true;
	}
	return !GetWorld()->LineTraceTestByChannel(Eye, Player->GetActorLocation(), ECC_Visibility, Query);
}

void ATTHunter::ObserveRoutesNear(const FIntPoint& PlayerCell)
{
	ATTHospital* Hospital = GetHospital();
	if (!Hospital || !Mind)
	{
		return;
	}
	const double T = Now();
	TArray<FIntPoint> Cells;
	Cells.Add(PlayerCell);
	for (const FIntPoint& Offset : Around)
	{
		Cells.Add(PlayerCell + Offset);
	}
	for (const FIntPoint& Cell : Cells)
	{
		if (!Hospital->GetDoorAt(Cell))
		{
			continue;
		}
		const double* Last = RouteObservedAt.Find(Cell);
		if (Last && T - *Last < 5.0)
		{
			continue;
		}
		RouteObservedAt.Add(Cell, T);
		const th::Stimulus S(th::StimulusKind::ObservedRoute, th::Subject::Player, T, std::nullopt, 1.0, ToStd(ATTHospital::DoorId(Cell)));
		if (BrainObserve(*Mind, S))
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] Memory: saw Joe at %s -> route evidence %.2f"), *ATTHospital::DoorId(Cell), RouteEvidence(Cell));
		}
	}
}

void ATTHunter::RebaselineDoor(const ATTDoor* Door)
{
	if (!Mind || !Door)
	{
		return;
	}
	try
	{
		Mind->Eyes.Inspect(ToStd(Door->GetId()), Door->IsOpen() ? "open" : "closed", ToCore(Door->GetActorLocation()), Now(), 1.0);
	}
	catch (...)
	{
	}
}

void ATTHunter::InspectDoors()
{
	ATTHospital* Hospital = GetHospital();
	if (!Hospital || !Mind)
	{
		return;
	}
	const FVector Eye = GetEyeLocation();
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Hospital->GetDoors())
	{
		ATTDoor* Door = Pair.Value;
		if (!Door || Door->IsMoving())
		{
			continue;
		}
		const FVector Point = Door->GetActorLocation() + FVector(0.f, 0.f, 120.f);
		const FVector To = Point - Eye;
		const float Dist = To.Size();
		if (Dist > 1100.f || (Dist > 300.f && FVector::DotProduct(GetActorForwardVector(), To.GetSafeNormal()) < 0.34f))
		{
			continue;
		}
		if (!CanSeePoint(Point, Door))
		{
			continue;
		}

		std::optional<th::Stimulus> Evidence;
		try
		{
			Evidence = Mind->Eyes.Inspect(ToStd(Door->GetId()), Door->IsOpen() ? "open" : "closed", ToCore(Door->GetActorLocation()), Now(), 1.0);
		}
		catch (...)
		{
			continue;
		}
		if (!Evidence)
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("[TT] Evidence: %s is now %s"), *Door->GetId(), Door->IsOpen() ? TEXT("open") : TEXT("closed"));
		const bool bCalm = State == ETTHunterState::Patrol || State == ETTHunterState::LookAround
			|| State == ETTHunterState::Search || State == ETTHunterState::Ambush;
		if (bCalm && BrainObserve(*Mind, *Evidence) && Mind->Brain.CurrentState() == th::State::Investigating)
		{
			// "That door was closed." He goes to see who opened it.
			StartInvestigate(Door->GetActorLocation(), 500.f);
		}
	}
}

void ATTHunter::UpdatePerception(float DeltaSeconds)
{
	float Distance = 0.f;
	float Visibility = 0.f;
	bSawPlayerThisFrame = CanSeePlayer(Distance, Visibility);

	ATTHospital* Hospital = GetHospital();
	if (bSawPlayerThisFrame)
	{
		const ATTPlayerCharacter* Player = GetPlayer();
		TimeSinceSeen = 0.f;
		LastKnown = Player->GetActorLocation();
		LastKnownVelocity = Player->GetVelocity();
		if (Hospital)
		{
			const FIntPoint JoeCell = Hospital->WorldToCell(LastKnown);
			ObserveRoutesNear(JoeCell);
			if (Hospital->IsIsolationCell(JoeCell))
			{
				Hospital->SetHunterKnowsIsolation();
			}
		}
		if (State == ETTHunterState::Pursue && Mind)
		{
			VisualReportTimer -= DeltaSeconds;
			if (VisualReportTimer <= 0.f)
			{
				VisualReportTimer = 0.3f;
				const double T = Now();
				BrainObserve(*Mind, th::Stimulus(th::StimulusKind::VisualContact, th::Subject::Player, T,
					th::LocationEstimate{ ToCore(LastKnown), 0.5, T }, FMath::Clamp(static_cast<double>(Visibility), 0.05, 1.0), ""));
			}
		}
	}
	else
	{
		TimeSinceSeen += DeltaSeconds;
	}

	DoorInspectTimer -= DeltaSeconds;
	if (DoorInspectTimer <= 0.f)
	{
		DoorInspectTimer = 0.4f;
		InspectDoors();
	}

	CompanionScanTimer -= DeltaSeconds;
	if (CompanionScanTimer <= 0.f && !bSawPlayerThisFrame)
	{
		CompanionScanTimer = 0.25f;
		ScanCompanions();
	}

	if (State == ETTHunterState::Pursue || State == ETTHunterState::Capturing || State == ETTHunterState::Stunned)
	{
		return;
	}

	if (!bSawPlayerThisFrame)
	{
		Awareness = FMath::Max(0.f, Awareness - DeltaSeconds * 0.12f);
		return;
	}

	const float EffectiveRange = FMath::Max(1.f, HunterTuning::SightRange * SightScale * Visibility);
	float Rate = Visibility * FMath::Clamp(1.8f - Distance / EffectiveRange, 0.25f, 1.8f) * 1.3f;
	if (State == ETTHunterState::Ambush || State == ETTHunterState::Investigate || State == ETTHunterState::Search)
	{
		Rate *= 1.8f;
	}
	Awareness += Rate * DeltaSeconds;
	if (Distance < 330.f)
	{
		Awareness = 1.f;
	}

	if (Awareness >= 1.f)
	{
		StartPursue();
	}
	else if (Awareness >= 0.3f && (State == ETTHunterState::Patrol || State == ETTHunterState::LookAround
		|| State == ETTHunterState::Ambush || State == ETTHunterState::SetTrap))
	{
		SuspiciousLocation = LastKnown;
		bSuspiciousFaint = false;
		SetState(ETTHunterState::Suspicious);
	}
	else if (State == ETTHunterState::Suspicious)
	{
		SuspiciousLocation = LastKnown;
		bSuspiciousFaint = false;
	}
}

void ATTHunter::OnNoiseHeard(const FVector& Location, ETTNoise Kind)
{
	if (!Mind || State == ETTHunterState::Dormant || State == ETTHunterState::Capturing || State == ETTHunterState::Stunned)
	{
		return;
	}

	tp::SoundKind SoundKind = tp::SoundKind::WalkFootstep;
	double Energy = 1.0;
	ToCore(Kind, SoundKind, Energy);

	const bool bClearPath = CanSeePoint(Location + FVector(0.f, 0.f, 50.f), nullptr);
	std::optional<th::Stimulus> Heard;
	try
	{
		Heard = Mind->Ears.Hear(tp::SoundEvent{ SoundKind, ToCore(Location), Now(), Energy },
			tp::ListenerContext{ ToCore(GetEyeLocation()), bClearPath ? 1.0 : 0.45, 0.0 });
	}
	catch (...)
	{
		return;
	}
	if (!Heard || !Heard->estimate)
	{
		return;
	}

	// GDD §9.2: an estimate with an uncertainty radius, never Joe's exact position.
	const FVector Estimated = FromCore(Heard->estimate->center);
	const float RadiusCm = static_cast<float>(Heard->estimate->radiusMeters * 100.0);
	const float Strength = static_cast<float>(Heard->strength);

	if (Kind == ETTNoise::Glass)
	{
		Aggression = FMath::Min(1.f, Aggression + 0.04f);
	}

	if (State == ETTHunterState::Pursue)
	{
		// Lost sight of him and something breaks over there: he can be fooled.
		if (LostSightTime > 0.3f && Strength >= 0.2f)
		{
			StartInvestigate(Estimated, RadiusCm);
		}
		return;
	}
	if ((State == ETTHunterState::CheckLocker && bLockerKnown) || State == ETTHunterState::Interrogate)
	{
		return;
	}
	if (State == ETTHunterState::PursueCompanion)
	{
		// Busy with the one he can see; only a loud sound pulls him away.
		if (Strength < 0.6f)
		{
			return;
		}
		if (ATTCompanion* Companion = TargetCompanion.Get())
		{
			Companion->OnChasedByHunter(false);
		}
	}

	const bool bInvestigate = BrainObserve(*Mind, *Heard)
		&& Strength >= static_cast<float>(Mind->Tuning.soundThreshold)
		&& Mind->Brain.CurrentState() == th::State::Investigating;
	if (bInvestigate)
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] Heard %s (strength %.2f, radius %.1fm) -> investigate"), UTF8_TO_TCHAR(Heard->perceivedType.c_str()), Strength, RadiusCm / 100.f);
		Awareness = FMath::Min(0.9f, Awareness + 0.2f);
		StartInvestigate(Estimated, RadiusCm);
	}
	else if (Strength >= HunterTuning::FaintSoundStrength && State != ETTHunterState::Investigate && State != ETTHunterState::Suspicious)
	{
		SuspiciousLocation = Estimated;
		bSuspiciousFaint = true;
		SetState(ETTHunterState::Suspicious);
	}
}

void ATTHunter::OnPlayerHid(ATTLocker* Locker)
{
	if (!Locker || State == ETTHunterState::Dormant || State == ETTHunterState::Capturing)
	{
		return;
	}
	// He knows only if he saw it: Joe himself, the locker door moving, or he was right behind him.
	const bool bSawJoe = TimeSinceSeen < 0.6f;
	const FVector LockerPoint = Locker->GetActorLocation() + FVector(0.f, 0.f, 150.f);
	const FVector ToLocker = LockerPoint - GetEyeLocation();
	const bool bSawDoorMove = ToLocker.Size() < 1400.f
		&& FVector::DotProduct(GetActorForwardVector(), ToLocker.GetSafeNormal()) > 0.3f
		&& CanSeePoint(LockerPoint, Locker);
	const bool bRightBehind = State == ETTHunterState::Pursue && LostSightTime < 1.2f && ToLocker.Size() < 900.f;

	if (bSawJoe || bSawDoorMove || bRightBehind)
	{
		ObserveHideSpot(Locker, 1.0);
		bLockerKnown = true;
		StartCheckLocker(Locker);
	}
}

void ATTHunter::OnPlayerLeftLocker(ATTLocker* Locker)
{
	float Distance = 0.f;
	float Visibility = 0.f;
	if (Locker && CanSeePlayer(Distance, Visibility))
	{
		ObserveHideSpot(Locker, 1.0);
	}
}

// ---------------------------------------------------------------------------
// Movement

bool ATTHunter::MoveTo(const FVector& Goal)
{
	Path.Reset();
	PathIndex = 0;
	DoorWait = 0.f;
	StuckTime = 0.f;

	ATTHospital* Hospital = GetHospital();
	if (!Hospital)
	{
		return false;
	}

	const FVector Start = GetActorLocation();
	const AActor* Player = GetPlayer();

	const FIntPoint GoalCell = Hospital->WorldToCell(FVector(Goal.X, Goal.Y, Goal.Z));
	const bool bSameFloor = Hospital->FloorOfLocation(Start) == Hospital->FloorOfCell(GoalCell);
	const FVector Target(Goal.X, Goal.Y, bSameFloor ? Start.Z : Hospital->CellToWorld(GoalCell).Z + 100.f);
	if (bSameFloor && IsDirectPathClear(GetWorld(), this, Player, Start, Target))
	{
		Path.Add(Target);
		return true;
	}

	TArray<FIntPoint> Cells;
	if (!Hospital->FindPath(Hospital->WorldToCell(Start), GoalCell, Cells, true))
	{
		return false;
	}

	TArray<FVector> Raw;
	for (int32 i = 1; i < Cells.Num(); ++i)
	{
		Raw.Add(Hospital->CellToWorld(Cells[i], 100.f));
	}
	if (Raw.Num() == 0)
	{
		Raw.Add(Hospital->CellToWorld(Cells[0], 100.f));
	}
	if (Hospital->IsWalkable(GoalCell))
	{
		Raw.Last() = Target;
	}

	// String-pull the grid path so he walks like a person, not along tiles.
	FVector Current = Start;
	int32 i = 0;
	while (i < Raw.Num())
	{
		int32 Best = i;
		for (int32 j = Raw.Num() - 1; j > i; --j)
		{
			if (Hospital->FloorOfLocation(Current) == Hospital->FloorOfLocation(Raw[j]) && IsDirectPathClear(GetWorld(), this, Player, Current, Raw[j]))
			{
				Best = j;
				break;
			}
		}
		Path.Add(Raw[Best]);
		Current = Raw[Best];
		i = Best + 1;
	}
	return true;
}

bool ATTHunter::FollowPath(float DeltaSeconds, float MoveSpeed)
{
	if (Path.Num() == 0 || PathIndex >= Path.Num())
	{
		return true;
	}
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	if (DoorWait > 0.f)
	{
		DoorWait -= DeltaSeconds;
		return false;
	}

	const FVector Loc = GetActorLocation();
	FVector To = Path[PathIndex] - Loc;
	// Ladder down into the tunnels (or back up): no stairs, he climbs.
	if (FMath::Abs(To.Z) > 250.f && To.Size2D() < 160.f)
	{
		SetActorLocation(FVector(Path[PathIndex].X, Path[PathIndex].Y, Path[PathIndex].Z), false, nullptr, ETeleportType::TeleportPhysics);
		TTAudio::PlayAt(this, ETTSfx::Clank, GetActorLocation(), 1.f);
		DoorWait = 0.8f;
		++PathIndex;
		return PathIndex >= Path.Num();
	}
	To.Z = 0.f;
	const bool bLast = PathIndex == Path.Num() - 1;
	if (To.Size() < (bLast ? 60.f : 90.f))
	{
		++PathIndex;
		StuckTime = 0.f;
		return PathIndex >= Path.Num();
	}

	const FVector Dir = To.GetSafeNormal();
	if (ATTHospital* Hospital = GetHospital())
	{
		const float SegmentLength = To.Size();
		for (const float Ahead : { 0.f, 110.f, 210.f })
		{
			if (Ahead > SegmentLength + 40.f)
			{
				break;
			}
			const FIntPoint Cell = Hospital->WorldToCell(Loc + Dir * Ahead);
			ATTDoor* Door = Hospital->GetDoorAt(Cell);
			if (Door && !Door->IsOpen())
			{
				Door->SetOpen(true, false, this);
				RebaselineDoor(Door);
				if (State != ETTHunterState::Pursue)
				{
					PendingCloseDoor = Cell;
				}
				DoorWait = 0.4f;
				return false;
			}
		}
	}

	AddMovementInput(Dir, 1.f);

	if (GetVelocity().Size2D() < 20.f)
	{
		StuckTime += DeltaSeconds;
		if (StuckTime > 1.2f)
		{
			StuckTime = 0.f;
			++PathIndex;
			return PathIndex >= Path.Num();
		}
	}
	else
	{
		StuckTime = 0.f;
	}
	return false;
}

void ATTHunter::UpdateDoorClosing()
{
	if (PendingCloseDoor.X < 0)
	{
		return;
	}
	ATTHospital* Hospital = GetHospital();
	ATTDoor* Door = Hospital ? Hospital->GetDoorAt(PendingCloseDoor) : nullptr;
	if (!Door || !Door->IsOpen())
	{
		PendingCloseDoor = FIntPoint(-1, -1);
		return;
	}
	const float Dist = FVector::Dist2D(Door->GetActorLocation(), GetActorLocation());
	if (Dist < 260.f)
	{
		return;
	}
	PendingCloseDoor = FIntPoint(-1, -1);

	const FVector DoorLoc = Door->GetActorLocation();
	const FVector Me = GetActorLocation();
	FVector Prev = Me;
	for (int32 i = PathIndex; i < Path.Num(); ++i)
	{
		const FVector A(Prev.X, Prev.Y, DoorLoc.Z);
		const FVector B(Path[i].X, Path[i].Y, DoorLoc.Z);
		if (FMath::PointDistToSegment(DoorLoc, A, B) < 160.f)
		{
			return; // He still has to go through it.
		}
		Prev = Path[i];
	}

	const bool bCalm = State == ETTHunterState::Patrol || State == ETTHunterState::Search
		|| State == ETTHunterState::LookAround || State == ETTHunterState::SetTrap;
	// He closes doors behind him. If one is open the next time he looks, someone opened it.
	if (bCalm && Dist < 450.f && FMath::FRand() < 0.6f)
	{
		Door->SetOpen(false, false, this);
		RebaselineDoor(Door);
		UE_LOG(LogTemp, Log, TEXT("[TT] Hunter closed %s behind him"), *Door->GetId());
	}
}

void ATTHunter::FaceTowards(const FVector& Target, float DeltaSeconds, float Rate)
{
	const FVector To = Target - GetActorLocation();
	if (To.SizeSquared2D() < 1.f)
	{
		return;
	}
	const FRotator Desired(0.f, To.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, Rate));
}

void ATTHunter::UpdateFootsteps(float DeltaSeconds)
{
	const float CurrentSpeed = GetVelocity().Size2D();
	if (CurrentSpeed < 30.f)
	{
		return;
	}
	const bool bFast = State == ETTHunterState::Pursue;
	const float Stride = bFast ? 215.f : 150.f;
	StepDistance += CurrentSpeed * DeltaSeconds;
	if (StepDistance >= Stride)
	{
		StepDistance = 0.f;
		const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, 90.f);
		TTAudio::PlayAt(this, ETTSfx::HunterStep, Feet, bFast ? 1.f : 0.75f);
	}
}

// ---------------------------------------------------------------------------
// Behaviour

void ATTHunter::StartPatrol()
{
	TargetLocker.Reset();
	bLockerKnown = false;

	if (TryStartTrap() || TryStartAmbush())
	{
		return;
	}

	ATTHospital* Hospital = GetHospital();
	if (!Hospital || !Mind)
	{
		return;
	}

	// GDD §16: base routes, weighted by what he has seen Joe use (HunterCore::ChoosePatrolRoute).
	std::vector<std::string> Routes;
	TMap<FString, FIntPoint> RouteCells;
	const TArray<FIntPoint>& Points = Hospital->GetPatrolPoints();
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FString Id = FString::Printf(TEXT("patrol_%d"), i);
		Routes.push_back(ToStd(Id));
		RouteCells.Add(Id, Points[i]);
	}
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Hospital->GetDoors())
	{
		const FString Id = ATTHospital::DoorId(Pair.Key);
		Routes.push_back(ToStd(Id));
		RouteCells.Add(Id, Pair.Key);
	}
	if (Routes.empty())
	{
		return;
	}

	FIntPoint TargetCell = Points.Num() > 0 ? Points[0] : Hospital->GetHunterStartCell();
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		FString Choice;
		try
		{
			Choice = UTF8_TO_TCHAR(Mind->Brain.ChoosePatrolRoute(Routes).c_str());
		}
		catch (...)
		{
			break;
		}
		const FIntPoint* Cell = RouteCells.Find(Choice);
		if (!Cell)
		{
			continue;
		}
		TargetCell = *Cell;
		const FVector Focus = TimeSinceSeen < 240.f ? LastKnown : GetActorLocation();
		if (FMath::Abs(Hospital->FloorOfCell(TargetCell) - Hospital->FloorOfLocation(Focus)) > 1 && FMath::FRand() > 0.2f && Attempt < 5)
		{
			continue;
		}
		if (Hospital->GetDoorAt(TargetCell))
		{
			// Stand beside the doorway, not inside it.
			TArray<FIntPoint> Beside;
			for (int32 k = 0; k < 4; ++k)
			{
				const FIntPoint C = TargetCell + Around[k];
				if (Hospital->IsWalkable(C) && !Hospital->GetDoorAt(C))
				{
					Beside.Add(C);
				}
			}
			if (Beside.Num() > 0)
			{
				TargetCell = Beside[FMath::RandRange(0, Beside.Num() - 1)];
			}
		}
		if (FVector::Dist2D(Hospital->CellToWorld(TargetCell), GetActorLocation()) > 250.f)
		{
			break;
		}
	}

	Awareness = FMath::Min(Awareness, 0.25f);
	PatrolTarget = Hospital->CellToWorld(TargetCell);
	MoveTo(PatrolTarget);
	SetState(ETTHunterState::Patrol);
}

void ATTHunter::ResumePatrol()
{
	MoveTo(PatrolTarget);
	SetState(ETTHunterState::Patrol);
}

bool ATTHunter::TryStartTrap()
{
	ATTHospital* Hospital = GetHospital();
	if (!Hospital || TrapCooldown > 0.f || Aggression < 0.3f || Hospital->HasLockedDoor())
	{
		return false;
	}

	// GDD §14 "bloquear uma rota": the door Joe keeps using, but never one that would trap him for good.
	FIntPoint Best(-1, -1);
	float BestScore = 1.5f;
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Hospital->GetDoors())
	{
		const float Score = RouteEvidence(Pair.Key);
		if (Score >= BestScore && Hospital->IsConnectedWithout(Pair.Key))
		{
			BestScore = Score;
			Best = Pair.Key;
		}
	}
	if (Best.X < 0)
	{
		return false;
	}

	FIntPoint Approach = Best;
	float Closest = TNumericLimits<float>::Max();
	for (int32 k = 0; k < 4; ++k)
	{
		const FIntPoint C = Best + Around[k];
		if (!Hospital->IsWalkable(C) || Hospital->GetDoorAt(C))
		{
			continue;
		}
		const float D = FVector::Dist2D(Hospital->CellToWorld(C), GetActorLocation());
		if (D < Closest)
		{
			Closest = D;
			Approach = C;
		}
	}

	TrapDoor = Best;
	TrapCooldown = 120.f;
	UE_LOG(LogTemp, Log, TEXT("[TT] Trap: going to block %s (evidence %.2f, aggression %.2f)"), *ATTHospital::DoorId(Best), BestScore, Aggression);
	MoveTo(Hospital->CellToWorld(Approach));
	SetState(ETTHunterState::SetTrap);
	return true;
}

bool ATTHunter::TryStartAmbush()
{
	ATTHospital* Hospital = GetHospital();
	if (!Hospital || AmbushCooldown > 0.f || Aggression < 0.2f)
	{
		return false;
	}

	FIntPoint Best(-1, -1);
	float BestScore = 1.f;
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Hospital->GetDoors())
	{
		const float Score = RouteEvidence(Pair.Key);
		if (Score >= BestScore)
		{
			BestScore = Score;
			Best = Pair.Key;
		}
	}
	if (Best.X < 0)
	{
		return false;
	}
	// GDD §15: ambushes must stay rare.
	if (FMath::FRand() > 0.35f + 0.4f * Aggression)
	{
		AmbushCooldown = 20.f;
		return false;
	}

	// Beside the doorway, around the corner from where Joe will come through.
	TArray<FIntPoint> Corners;
	for (int32 k = 4; k < 8; ++k)
	{
		const FIntPoint C = Best + Around[k];
		if (Hospital->IsWalkable(C) && !Hospital->GetDoorAt(C))
		{
			Corners.Add(C);
		}
	}
	if (Corners.Num() == 0)
	{
		return false;
	}

	AmbushDoor = Best;
	AmbushPoint = Hospital->CellToWorld(Corners[FMath::RandRange(0, Corners.Num() - 1)]);
	bAmbushSettled = false;
	AmbushTimeLeft = 22.f + 15.f * Aggression;
	AmbushCooldown = 75.f;
	UE_LOG(LogTemp, Log, TEXT("[TT] Ambush: heading to %s (evidence %.2f)"), *ATTHospital::DoorId(Best), BestScore);
	MoveTo(AmbushPoint);
	SetState(ETTHunterState::Ambush);
	return true;
}

void ATTHunter::StartInvestigate(const FVector& Location, float RadiusCm)
{
	InvestigateLocation = Location;
	InvestigateRadius = FMath::Max(300.f, RadiusCm);
	MoveTo(Location);
	SetState(ETTHunterState::Investigate);
}

void ATTHunter::StartSearch(const FVector& Center, float Duration)
{
	SearchCenter = Center;
	SearchTimeLeft = Duration;
	SearchQueue.Reset();

	if (ATTHospital* Hospital = GetHospital())
	{
		// GDD §11: lockers he has seen Joe use are more likely to be opened.
		for (const TObjectPtr<ATTLocker>& Locker : Hospital->GetLockers())
		{
			if (!Locker)
			{
				continue;
			}
			const float Dist = FVector::Dist2D(Locker->GetActorLocation(), Center);
			if (Dist > 1500.f)
			{
				continue;
			}
			const float Chance = HideSpotSuspicion(Locker) / 100.f + (Dist < 650.f ? 0.25f : 0.f);
			if (FMath::FRand() < Chance)
			{
				FSearchTarget T;
				T.Location = Locker->GetFrontLocation();
				T.Locker = Locker.Get();
				SearchQueue.Add(T);
			}
		}

		const int32 Radius = FMath::Clamp(FMath::RoundToInt(InvestigateRadius / ATTHospital::CellSize), 1, 4);
		TArray<FIntPoint> Cells = Hospital->GetWalkableCellsNear(Hospital->WorldToCell(Center), Radius);
		for (int32 i = Cells.Num() - 1; i > 0; --i)
		{
			Cells.Swap(i, FMath::RandRange(0, i));
		}
		for (int32 i = 0; i < FMath::Min(3, Cells.Num()); ++i)
		{
			FSearchTarget T;
			T.Location = Hospital->CellToWorld(Cells[i]);
			SearchQueue.Add(T);
		}

		SearchQueue.Sort([&Center](const FSearchTarget& A, const FSearchTarget& B)
		{
			return FVector::DistSquared2D(A.Location, Center) < FVector::DistSquared2D(B.Location, Center);
		});
		const int32 MaxTargets = SearchTargets + FMath::RoundToInt(2.f * Aggression);
		if (SearchQueue.Num() > MaxTargets)
		{
			SearchQueue.SetNum(MaxTargets);
		}
	}

	LookDuration = 1.8f;
	AfterLook = ETTHunterState::Search;
	SetState(ETTHunterState::LookAround);
}

void ATTHunter::NextSearchTarget()
{
	if (SearchTimeLeft <= 0.f || SearchQueue.Num() == 0)
	{
		StartPatrol();
		return;
	}
	CurrentTarget = SearchQueue[0];
	SearchQueue.RemoveAt(0);
	MoveTo(CurrentTarget.Location);
	SetState(ETTHunterState::Search);
}

void ATTHunter::StartCheckLocker(ATTLocker* Locker)
{
	TargetLocker = Locker;
	LockerTimer = 0.f;
	MoveTo(Locker->GetFrontLocation());
	SetState(ETTHunterState::CheckLocker);
}

void ATTHunter::StartPursue()
{
	if (State == ETTHunterState::Pursue)
	{
		return;
	}
	if (State == ETTHunterState::Ambush && RouteEvidence(AmbushDoor) >= 1.5f)
	{
		SpeechCooldown = 0.f;
		Say(TEXT("Você sempre escolhe essa porta."));
	}
	else
	{
		Say(TimesLostJoe == 0 ? TEXT("Pare.") : TEXT("Achei você."), 0.6f);
	}
	TTAudio::Play2D(this, ETTSfx::Sting, 0.35f);
	Awareness = 1.f;
	LostSightTime = 0.f;
	RepathTimer = 0.f;
	VisualReportTimer = 0.f;
	SetState(ETTHunterState::Pursue);
}

void ATTHunter::LosePursuit()
{
	if (Mind)
	{
		BrainObserve(*Mind, th::Stimulus(th::StimulusKind::VisualLost, th::Subject::Player, Now(), std::nullopt, 1.0, ""));
	}
	++TimesLostJoe;
	Aggression = FMath::Min(1.f, Aggression + 0.15f);
	Awareness = 0.6f;
	InvestigateRadius = 600.f;
	UE_LOG(LogTemp, Log, TEXT("[TT] Lost Joe (%d times). Aggression %.2f"), TimesLostJoe, Aggression);
	Say(TEXT("Eu sei que você está aí."), 0.5f);
	StartSearch(LastKnown, SearchSeconds * (1.f + 0.5f * Aggression));
}

void ATTHunter::Stun(float Seconds, bool bExplosion)
{
	if (State == ETTHunterState::Dormant || State == ETTHunterState::Capturing)
	{
		return;
	}
	if (Seconds >= 2.f)
	{
		if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
		{
			GM->OnHunterStunned();
		}
	}
	Aggression = FMath::Min(1.f, Aggression + (bExplosion ? 0.3f : 0.2f));
	if (State == ETTHunterState::PursueCompanion)
	{
		if (ATTCompanion* Companion = TargetCompanion.Get())
		{
			Companion->OnSavedByJoe();
		}
	}
	StunTimeLeft = FMath::Max(StunTimeLeft, Seconds);
	Path.Reset();
	GetCharacterMovement()->StopMovementImmediately();
	UE_LOG(LogTemp, Log, TEXT("[TT] Hunter stunned %.1fs (%s). Aggression %.2f"), Seconds, bExplosion ? TEXT("explosion") : TEXT("hit"), Aggression);
	if (State != ETTHunterState::Stunned)
	{
		SetState(ETTHunterState::Stunned);
	}
}

void ATTHunter::ReachJoe()
{
	ATTPlayerCharacter* Player = GetPlayer();
	if (!Player)
	{
		Capture();
		return;
	}
	// Easy/Normal: blows until Joe is badly hurt. Hard: one blow. Punitive: none.
	static const int32 StrikeUntilInjury[] = { 3, 3, 2, 0 };
	const int32 Limit = StrikeUntilInjury[static_cast<int32>(Difficulty)];
	if (Player->GetInjury() < Limit && !Player->IsCollapsed() && StrikeRecover > 0.f)
	{
		return; // Still recovering from the last blow: keep pressing, no grab yet.
	}
	if (Player->GetInjury() < Limit && !Player->IsCollapsed())
	{
		Player->HunterStrike(GetActorLocation());
		StrikeRecover = 2.5f;
		SpeechCooldown = 0.f;
		Say(TEXT("Não corra."), 0.5f);
		Stun(1.1f, false);
		Aggression = FMath::Max(0.f, Aggression - 0.2f);
		return;
	}
	SpeechCooldown = 0.f;
	Say(TEXT("Acabou."));
	Capture();
}

void ATTHunter::ScanCompanions()
{
	const bool bFree = State == ETTHunterState::Patrol || State == ETTHunterState::LookAround || State == ETTHunterState::Search
		|| State == ETTHunterState::Investigate || State == ETTHunterState::Ambush || State == ETTHunterState::Suspicious
		|| State == ETTHunterState::SetTrap;
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	if (!bFree || !GM)
	{
		return;
	}
	const FVector Eye = GetEyeLocation();
	for (ATTCompanion* Companion : GM->GetCompanions())
	{
		if (!Companion || !Companion->IsVisibleTarget())
		{
			continue;
		}
		const FVector Target = Companion->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const FVector To = Target - Eye;
		const float Dist = To.Size();
		const float Range = HunterTuning::SightRange * SightScale * Companion->GetVisibility();
		const bool bClose = Dist < HunterTuning::CloseSense;
		if (Dist > Range && !bClose)
		{
			continue;
		}
		if (!bClose && FVector::DotProduct(GetActorForwardVector(), To.GetSafeNormal()) < 0.5f)
		{
			continue;
		}
		if (GM->IsSightBlockedBySmoke(Eye, Target) || !CanSeePoint(Target, Companion))
		{
			continue;
		}
		StartPursueCompanion(Companion);
		return;
	}
}

void ATTHunter::StartPursueCompanion(ATTCompanion* Companion)
{
	TargetCompanion = Companion;
	CompanionLostTime = 0.f;
	CompanionLastSeen = Companion->GetActorLocation();
	RepathTimer = 0.f;
	UE_LOG(LogTemp, Log, TEXT("[TT] Hunter spotted %s"), *Companion->GetDisplayName());
	Companion->OnChasedByHunter(true);
	SetState(ETTHunterState::PursueCompanion);
}

void ATTHunter::TakeCompanion(ATTCompanion* Companion)
{
	if (!Companion)
	{
		return;
	}
	bInterrogationInfo = Companion->TakenByHunter(InterrogationLocation, InterrogationHideSpot);
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->OnCompanionTaken(Companion);
	}
	TargetCompanion.Reset();
	TargetLocker.Reset();
	bLockerKnown = false;
	Path.Reset();
	GetCharacterMovement()->StopMovementImmediately();
	SpeechCooldown = 0.f;
	Say(TEXT("Onde ele está?"));
	SetState(ETTHunterState::Interrogate);
}

void ATTHunter::Capture()
{
	SetState(ETTHunterState::Capturing);
	Path.Reset();
	GetCharacterMovement()->StopMovementImmediately();
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->PlayerCaptured(this);
	}
}

// ---------------------------------------------------------------------------
// Tick

void ATTHunter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (CapePivot)
	{
		// The cape trails behind at speed and settles when he stops.
		const float Speed = GetVelocity().Size2D();
		const float Target = -FMath::Clamp(Speed / 520.f, 0.f, 1.f) * 38.f - 3.f;
		CapePitch = FMath::FInterpTo(CapePitch, Target + FMath::Sin(GetWorld()->GetTimeSeconds() * 7.f) * Speed * 0.006f, DeltaSeconds, 5.f);
		CapePivot->SetRelativeRotation(FRotator(CapePitch, 0.f, 0.f));
	}
	if (State == ETTHunterState::Dormant)
	{
		return;
	}

	StateTime += DeltaSeconds;
	SpeechCooldown -= DeltaSeconds;
	StrikeRecover -= DeltaSeconds;
	AmbushCooldown -= DeltaSeconds;
	TrapCooldown -= DeltaSeconds;
	if (State != ETTHunterState::Pursue)
	{
		Aggression = FMath::Max(0.f, Aggression - DeltaSeconds * 0.003f);
	}
	if (Mind)
	{
		try
		{
			Mind->Brain.AdvanceTo(Now());
		}
		catch (...)
		{
		}
	}

	UpdatePerception(DeltaSeconds);
	UpdateDoorClosing();

	ATTPlayerCharacter* Player = GetPlayer();
	ATTHospital* Hospital = GetHospital();

	switch (State)
	{
	case ETTHunterState::Patrol:
		if (FollowPath(DeltaSeconds, Speed(HunterTuning::PatrolSpeed)))
		{
			LookDuration = FMath::FRandRange(2.5f, 4.f);
			AfterLook = ETTHunterState::Patrol;
			SetState(ETTHunterState::LookAround);
		}
		break;

	case ETTHunterState::LookAround:
	{
		const float Yaw = LookBaseYaw + FMath::Sin(StateTime * 1.3f) * 75.f;
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0.f, Yaw, 0.f), DeltaSeconds, 5.f));
		if (AfterLook == ETTHunterState::Search)
		{
			SearchTimeLeft -= DeltaSeconds;
			// GDD §20: outside the hiding spot... "Joe." ...then he leaves.
			if (Player && Player->IsHidden() && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < 500.f)
			{
				Say(TEXT("Joe."), 0.02f);
			}
		}
		if (StateTime >= LookDuration)
		{
			if (AfterLook == ETTHunterState::Search)
			{
				NextSearchTarget();
			}
			else
			{
				StartPatrol();
			}
		}
		break;
	}

	case ETTHunterState::Suspicious:
		GetCharacterMovement()->StopActiveMovement();
		FaceTowards(SuspiciousLocation, DeltaSeconds, 4.f);
		if (StateTime > 1.3f)
		{
			if (bSuspiciousFaint && Awareness < 0.3f)
			{
				// A faint sound: he looks, listens, and goes back to what he was doing.
				if (PreviousState == ETTHunterState::Search && SearchTimeLeft > 0.f)
				{
					NextSearchTarget();
				}
				else
				{
					ResumePatrol();
				}
			}
			else
			{
				StartInvestigate(SuspiciousLocation, 400.f);
			}
		}
		break;

	case ETTHunterState::Investigate:
		if (bSawPlayerThisFrame && FVector::Dist2D(LastKnown, InvestigateLocation) > 250.f && StateTime > 0.5f)
		{
			StartInvestigate(LastKnown, 300.f);
		}
		else if (FollowPath(DeltaSeconds, Speed(HunterTuning::InvestigateSpeed)))
		{
			StartSearch(InvestigateLocation, SearchSeconds * 0.6f * (1.f + 0.5f * Aggression));
		}
		break;

	case ETTHunterState::Search:
		SearchTimeLeft -= DeltaSeconds;
		if (FollowPath(DeltaSeconds, Speed(HunterTuning::SearchSpeed)))
		{
			if (ATTLocker* Locker = CurrentTarget.Locker.Get())
			{
				bLockerKnown = false;
				StartCheckLocker(Locker);
			}
			else
			{
				LookDuration = FMath::FRandRange(1.5f, 2.5f);
				AfterLook = ETTHunterState::Search;
				SetState(ETTHunterState::LookAround);
			}
		}
		break;

	case ETTHunterState::CheckLocker:
	{
		SearchTimeLeft -= DeltaSeconds;
		ATTLocker* Locker = TargetLocker.Get();
		if (!Locker)
		{
			NextSearchTarget();
			break;
		}
		if (FollowPath(DeltaSeconds, Speed(bLockerKnown ? 330.f : HunterTuning::SearchSpeed)))
		{
			FaceTowards(Locker->GetActorLocation(), DeltaSeconds, 8.f);
			LockerTimer += DeltaSeconds;
			if (LockerTimer > 0.8f)
			{
				Locker->FlapDoor();
				if (ATTCompanion* Inside = Cast<ATTCompanion>(Locker->GetOccupant()))
				{
					TakeCompanion(Inside);
				}
				else if (Locker->IsOccupied())
				{
					ObserveHideSpot(Locker, 1.0);
					SpeechCooldown = 0.f;
					Say(HideSpotSuspicion(Locker) >= 60.f ? TEXT("Você sempre escolhe esse armário.") : TEXT("Acabou."));
					Capture();
				}
				else
				{
					// He can be wrong (GDD §13).
					TargetLocker.Reset();
					bLockerKnown = false;
					LookDuration = 1.2f;
					AfterLook = ETTHunterState::Search;
					SetState(ETTHunterState::LookAround);
				}
			}
		}
		break;
	}

	case ETTHunterState::Ambush:
		if (!bAmbushSettled)
		{
			if (FollowPath(DeltaSeconds, Speed(HunterTuning::InvestigateSpeed)))
			{
				bAmbushSettled = true;
				if (Hospital)
				{
					const int32 Killed = Hospital->SetLampsNear(AmbushPoint, 700.f, false);
					UE_LOG(LogTemp, Log, TEXT("[TT] Ambush set by %s, %d lamp(s) out"), *ATTHospital::DoorId(AmbushDoor), Killed);
				}
			}
		}
		else
		{
			GetCharacterMovement()->StopActiveMovement();
			if (Hospital)
			{
				FaceTowards(Hospital->CellToWorld(AmbushDoor), DeltaSeconds, 3.f);
			}
			AmbushTimeLeft -= DeltaSeconds;
			if (AmbushTimeLeft <= 0.f)
			{
				UE_LOG(LogTemp, Log, TEXT("[TT] Ambush abandoned"));
				StartPatrol();
			}
		}
		break;

	case ETTHunterState::SetTrap:
		if (FollowPath(DeltaSeconds, Speed(HunterTuning::InvestigateSpeed)) && Hospital)
		{
			ATTDoor* Door = Hospital->GetDoorAt(TrapDoor);
			if (Door)
			{
				FaceTowards(Door->GetActorLocation(), DeltaSeconds, 6.f);
			}
			if (StateTime > 1.f || !Door)
			{
				if (Door)
				{
					Door->SetOpen(false, false, this);
					Door->SetLocked(true);
					RebaselineDoor(Door);
					TTAudio::PlayAt(this, ETTSfx::Clank, Door->GetActorLocation() + FVector(0.f, 0.f, 100.f), 1.f);
					UE_LOG(LogTemp, Log, TEXT("[TT] Trap: %s is now blocked"), *Door->GetId());
				}
				TrapDoor = FIntPoint(-1, -1);
				StartPatrol();
			}
		}
		break;

	case ETTHunterState::Pursue:
	{
		if (bSawPlayerThisFrame && Player)
		{
			LostSightTime = 0.f;
			RepathTimer -= DeltaSeconds;
			if (RepathTimer <= 0.f)
			{
				RepathTimer = 0.25f;
				MoveTo(LastKnown);
			}
			if (FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) < HunterTuning::CaptureDistance)
			{
				ReachJoe();
				break;
			}
		}
		else
		{
			if (LostSightTime == 0.f)
			{
				// Predict where he was heading (GDD §17).
				MoveTo(LastKnown + LastKnownVelocity.GetClampedToMaxSize(450.f) * 1.2f);
			}
			LostSightTime += DeltaSeconds;
		}

		const bool bArrived = FollowPath(DeltaSeconds, Speed(HunterTuning::PursueSpeed));
		if (!bSawPlayerThisFrame && (bArrived || LostSightTime > 4.f))
		{
			LosePursuit();
		}
		break;
	}

	case ETTHunterState::Stunned:
		GetCharacterMovement()->StopActiveMovement();
		StunTimeLeft -= DeltaSeconds;
		if (StunTimeLeft <= 0.f)
		{
			// He comes back angrier, and he knows roughly where it came from.
			if (bSawPlayerThisFrame || TimeSinceSeen < 1.f)
			{
				StartPursue();
			}
			else
			{
				InvestigateRadius = 600.f;
				StartSearch(LastKnown, SearchSeconds * (1.f + 0.5f * Aggression));
			}
		}
		break;

	case ETTHunterState::PursueCompanion:
	{
		ATTCompanion* Companion = TargetCompanion.Get();
		if (!Companion || !Companion->IsActive())
		{
			StartPatrol();
			break;
		}
		if (Companion->IsInLocker())
		{
			// He watched them get in.
			if (CompanionLostTime < 1.5f && Companion->GetHiddenLocker())
			{
				bLockerKnown = true;
				StartCheckLocker(Companion->GetHiddenLocker());
			}
			else
			{
				Companion->OnChasedByHunter(false);
				StartSearch(CompanionLastSeen, SearchSeconds * 0.6f);
			}
			break;
		}
		const FVector Target = Companion->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
		const bool bSees = CanSeePoint(Target, Companion) && !(GM && GM->IsSightBlockedBySmoke(GetEyeLocation(), Target));
		if (bSees)
		{
			CompanionLostTime = 0.f;
			CompanionLastSeen = Companion->GetActorLocation();
			RepathTimer -= DeltaSeconds;
			if (RepathTimer <= 0.f)
			{
				RepathTimer = 0.3f;
				MoveTo(CompanionLastSeen);
			}
		}
		else
		{
			CompanionLostTime += DeltaSeconds;
		}
		FollowPath(DeltaSeconds, Speed(HunterTuning::PursueSpeed));
		if (FVector::Dist2D(Companion->GetActorLocation(), GetActorLocation()) < HunterTuning::CaptureDistance)
		{
			TakeCompanion(Companion);
		}
		else if (CompanionLostTime > 3.5f)
		{
			Companion->OnChasedByHunter(false);
			UE_LOG(LogTemp, Log, TEXT("[TT] Hunter lost %s"), *Companion->GetDisplayName());
			StartSearch(CompanionLastSeen, SearchSeconds * 0.6f);
		}
		break;
	}

	case ETTHunterState::Interrogate:
		GetCharacterMovement()->StopActiveMovement();
		if (StateTime > 2.5f)
		{
			// GDD §49: companions can give Joe away. Only what they actually saw.
			if (bInterrogationInfo)
			{
				if (!InterrogationHideSpot.IsEmpty() && Hospital)
				{
					for (const TObjectPtr<ATTLocker>& L : Hospital->GetLockers())
					{
						if (L && L->GetId() == InterrogationHideSpot)
						{
							ObserveHideSpot(L.Get(), 1.0);
						}
					}
				}
				UE_LOG(LogTemp, Log, TEXT("[TT] Interrogation: told where Joe was (%s, spot %s)"),
					*InterrogationLocation.ToCompactString(), InterrogationHideSpot.IsEmpty() ? TEXT("-") : *InterrogationHideSpot);
				Aggression = FMath::Min(1.f, Aggression + 0.1f);
				StartInvestigate(InterrogationLocation, 500.f);
			}
			else
			{
				StartPatrol();
			}
			bInterrogationInfo = false;
		}
		break;

	case ETTHunterState::Capturing:
		if (Player)
		{
			FaceTowards(Player->GetActorLocation(), DeltaSeconds, 10.f);
		}
		break;

	default:
		break;
	}

	UpdateFootsteps(DeltaSeconds);
}
