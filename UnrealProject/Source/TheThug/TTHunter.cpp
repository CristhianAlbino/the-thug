#include "TTHunter.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TTAudio.h"
#include "TTGameMode.h"
#include "TTHospital.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"
#include "TTShapes.h"

namespace HunterTuning
{
	constexpr float PatrolSpeed = 175.f;
	constexpr float SearchSpeed = 215.f;
	constexpr float InvestigateSpeed = 280.f;
	constexpr float PursueSpeed = 455.f;
	constexpr float SightRange = 2400.f;
	constexpr float PursueSightRange = 3200.f;
	constexpr float CloseSense = 230.f;
	constexpr float CaptureDistance = 145.f;
	constexpr float HearingScale = 2600.f;
}

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

void ATTHunter::BeginPlay()
{
	Super::BeginPlay();
	BuildVisual();
}

void ATTHunter::BuildVisual()
{
	USceneComponent* Root = GetCapsuleComponent();
	const FLinearColor Black(0.004f, 0.004f, 0.005f);
	const FLinearColor Suit(0.008f, 0.008f, 0.01f);

	TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, -13.f, -52.f), FVector(22.f, 22.f, 96.f), Suit, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 13.f, -52.f), FVector(22.f, 22.f, 96.f), Suit, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 32.f), FVector(40.f, 62.f, 82.f), Suit, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(-27.f, 0.f, -4.f), FVector(8.f, 100.f, 158.f), Black, FRotator(-5.f, 0.f, 0.f), false);
	TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(2.f, 0.f, 88.f), FVector(30.f, 28.f, 32.f), Black, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Cone(), FVector(0.f, -9.f, 108.f), FVector(8.f, 8.f, 16.f), Black, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Cone(), FVector(0.f, 9.f, 108.f), FVector(8.f, 8.f, 16.f), Black, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(14.f, -6.f, 90.f), FVector(4.f, 5.f, 2.5f), FLinearColor(0.8f, 0.85f, 0.9f), FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(14.f, 6.f, 90.f), FVector(4.f, 5.f, 2.5f), FLinearColor(0.8f, 0.85f, 0.9f), FRotator::ZeroRotator, false);

	UPointLightComponent* EyeGlow = NewObject<UPointLightComponent>(this);
	EyeGlow->SetupAttachment(Root);
	EyeGlow->SetRelativeLocation(FVector(24.f, 0.f, 90.f));
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

FString ATTHunter::GetStateName() const
{
	if (const UEnum* Enum = StaticEnum<ETTHunterState>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(State));
	}
	return FString();
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
		UE_LOG(LogTemp, Log, TEXT("[TT] Hunter %s -> %s"), *StaticEnum<ETTHunterState>()->GetNameStringByValue(static_cast<int64>(State)),
			*StaticEnum<ETTHunterState>()->GetNameStringByValue(static_cast<int64>(NewState)));
	}
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
// Perception

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
		const FVector Forward = GetActorForwardVector();
		if (FVector::DotProduct(Forward, ToPlayer.GetSafeNormal()) < CosHalfAngle)
		{
			return false;
		}
		const float Range = bChasing ? HunterTuning::PursueSightRange : HunterTuning::SightRange * OutVisibility;
		if (OutDistance > Range)
		{
			return false;
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

void ATTHunter::UpdatePerception(float DeltaSeconds)
{
	float Distance = 0.f;
	float Visibility = 0.f;
	bSawPlayerThisFrame = CanSeePlayer(Distance, Visibility);

	if (bSawPlayerThisFrame)
	{
		const ATTPlayerCharacter* Player = GetPlayer();
		TimeSinceSeen = 0.f;
		LastKnown = Player->GetActorLocation();
		LastKnownVelocity = Player->GetVelocity();
	}
	else
	{
		TimeSinceSeen += DeltaSeconds;
	}

	if (State == ETTHunterState::Pursue || State == ETTHunterState::Capturing)
	{
		return;
	}

	if (!bSawPlayerThisFrame)
	{
		Awareness = FMath::Max(0.f, Awareness - DeltaSeconds * 0.12f);
		return;
	}

	const float EffectiveRange = FMath::Max(1.f, HunterTuning::SightRange * Visibility);
	const float Rate = Visibility * FMath::Clamp(1.8f - Distance / EffectiveRange, 0.25f, 1.8f) * 1.3f;
	Awareness += Rate * DeltaSeconds;
	if (Distance < 330.f)
	{
		Awareness = 1.f;
	}

	if (Awareness >= 1.f)
	{
		StartPursue();
	}
	else if (Awareness >= 0.3f && (State == ETTHunterState::Patrol || State == ETTHunterState::LookAround))
	{
		SuspiciousLocation = LastKnown;
		SetState(ETTHunterState::Suspicious);
	}
	else if (State == ETTHunterState::Suspicious)
	{
		SuspiciousLocation = LastKnown;
	}
}

void ATTHunter::OnNoiseHeard(const FVector& Location, float Loudness)
{
	if (State == ETTHunterState::Dormant || State == ETTHunterState::Capturing)
	{
		return;
	}

	const FVector Eye = GetEyeLocation();
	const float Distance = FVector::Dist(Location, Eye);
	float Range = Loudness * HunterTuning::HearingScale;

	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTHunterHearing), false, this);
	if (const ATTPlayerCharacter* Player = GetPlayer())
	{
		Query.AddIgnoredActor(Player);
	}
	if (GetWorld()->LineTraceTestByChannel(Location + FVector(0.f, 0.f, 50.f), Eye, ECC_Visibility, Query))
	{
		Range *= 0.55f;
	}
	if (Distance > Range)
	{
		return;
	}

	// GDD §9.2: only an approximate position, never the exact one.
	const FVector2D Jitter = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal() * Distance * 0.12f;
	const FVector Estimated = Location + FVector(Jitter.X, Jitter.Y, 0.f);

	if (State == ETTHunterState::Pursue)
	{
		if (LostSightTime > 0.3f)
		{
			StartInvestigate(Estimated);
		}
		return;
	}
	if (State == ETTHunterState::CheckLocker && TargetLocker.IsValid() && TargetLocker->IsOccupied())
	{
		return;
	}

	Awareness = FMath::Min(0.9f, Awareness + Loudness * 0.5f);
	const bool bAlreadyAlert = State == ETTHunterState::Investigate || State == ETTHunterState::Search
		|| State == ETTHunterState::Suspicious || (State == ETTHunterState::LookAround && AfterLook == ETTHunterState::Search);
	if (Loudness >= 0.35f || bAlreadyAlert)
	{
		StartInvestigate(Estimated);
	}
	else
	{
		SuspiciousLocation = Estimated;
		SetState(ETTHunterState::Suspicious);
	}
}

void ATTHunter::OnPlayerHid(ATTLocker* Locker)
{
	if (!Locker || State == ETTHunterState::Dormant || State == ETTHunterState::Capturing)
	{
		return;
	}
	// Only if he actually saw (or was right behind) Joe when he got in. No mind reading.
	const bool bSawIt = TimeSinceSeen < 0.6f;
	const bool bRightBehind = State == ETTHunterState::Pursue && LostSightTime < 1.2f
		&& FVector::Dist(Locker->GetActorLocation(), GetActorLocation()) < 900.f;
	if (bSawIt || bRightBehind)
	{
		StartCheckLocker(Locker);
	}
}

// ---------------------------------------------------------------------------
// Movement

namespace
{
	bool IsDirectPathClear(const UWorld* World, const AActor* Self, const AActor* Ignore, const FVector& From, const FVector& To)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(TTHunterPath), false, Self);
		if (Ignore)
		{
			Query.AddIgnoredActor(Ignore);
		}
		const FVector A(From.X, From.Y, From.Z);
		const FVector B(To.X, To.Y, From.Z);
		return !World->SweepTestByChannel(A, B, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(38.f, 60.f), Query);
	}
}

bool ATTHunter::MoveTo(const FVector& Goal)
{
	Path.Reset();
	PathCells.Reset();
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
	FVector Target(Goal.X, Goal.Y, Start.Z);

	if (IsDirectPathClear(GetWorld(), this, Player, Start, Target))
	{
		Path.Add(Target);
		PathCells.Add(Hospital->WorldToCell(Target));
		return true;
	}

	TArray<FIntPoint> Cells;
	if (!Hospital->FindPath(Hospital->WorldToCell(Start), Hospital->WorldToCell(Target), Cells))
	{
		return false;
	}

	TArray<FVector> Raw;
	TArray<FIntPoint> RawCells;
	for (int32 i = 1; i < Cells.Num(); ++i)
	{
		Raw.Add(Hospital->CellToWorld(Cells[i], Start.Z));
		RawCells.Add(Cells[i]);
	}
	if (Raw.Num() == 0)
	{
		Raw.Add(Hospital->CellToWorld(Cells[0], Start.Z));
		RawCells.Add(Cells[0]);
	}
	if (Hospital->IsWalkable(Hospital->WorldToCell(Target)))
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
			if (IsDirectPathClear(GetWorld(), this, Player, Current, Raw[j]))
			{
				Best = j;
				break;
			}
		}
		Path.Add(Raw[Best]);
		PathCells.Add(RawCells[Best]);
		Current = Raw[Best];
		i = Best + 1;
	}
	return true;
}

bool ATTHunter::FollowPath(float DeltaSeconds, float Speed)
{
	if (Path.Num() == 0 || PathIndex >= Path.Num())
	{
		return true;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	if (DoorWait > 0.f)
	{
		DoorWait -= DeltaSeconds;
		return false;
	}

	const FVector Loc = GetActorLocation();
	FVector To = Path[PathIndex] - Loc;
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
		for (const float Ahead : { 0.f, 110.f, 210.f })
		{
			ATTDoor* Door = Hospital->GetDoorAt(Hospital->WorldToCell(Loc + Dir * Ahead));
			if (Door && !Door->IsOpen())
			{
				Door->SetOpen(true, false, this);
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
	const float Speed = GetVelocity().Size2D();
	if (Speed < 30.f)
	{
		return;
	}
	const bool bFast = State == ETTHunterState::Pursue;
	const float Stride = bFast ? 215.f : 150.f;
	StepDistance += Speed * DeltaSeconds;
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
	ATTHospital* Hospital = GetHospital();
	if (!Hospital || Hospital->GetPatrolPoints().Num() == 0)
	{
		SetState(ETTHunterState::LookAround);
		LookDuration = 3.f;
		AfterLook = ETTHunterState::Patrol;
		return;
	}

	const TArray<FIntPoint>& Points = Hospital->GetPatrolPoints();
	int32 Next = FMath::RandRange(0, Points.Num() - 1);
	if (Points.Num() > 1 && Next == PatrolIndex)
	{
		Next = (Next + 1) % Points.Num();
	}
	PatrolIndex = Next;
	Awareness = FMath::Min(Awareness, 0.25f);
	MoveTo(Hospital->CellToWorld(Points[PatrolIndex]));
	SetState(ETTHunterState::Patrol);
}

void ATTHunter::StartInvestigate(const FVector& Location)
{
	InvestigateLocation = Location;
	MoveTo(Location);
	SetState(ETTHunterState::Investigate);
}

void ATTHunter::StartSearch(const FVector& Center, float Duration)
{
	SearchCenter = Center;
	SearchTimeLeft = Duration;
	SearchQueue.Reset();

	ATTHospital* Hospital = GetHospital();
	if (Hospital)
	{
		// GDD §11: lockers Joe used before become more likely to be checked.
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
			const float Chance = Locker->GetSuspicion() / 100.f + (Dist < 650.f ? 0.25f : 0.f);
			if (FMath::FRand() < Chance)
			{
				FSearchTarget T;
				T.Location = Locker->GetFrontLocation();
				T.Locker = Locker.Get();
				SearchQueue.Add(T);
			}
		}

		TArray<FIntPoint> Cells = Hospital->GetWalkableCellsNear(Hospital->WorldToCell(Center), 3);
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
		if (SearchQueue.Num() > 5)
		{
			SearchQueue.SetNum(5);
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
	Say(TimesLostJoe == 0 ? TEXT("Pare.") : TEXT("Achei você."), 0.6f);
	TTAudio::Play2D(this, ETTSfx::Sting, 0.35f);
	Awareness = 1.f;
	LostSightTime = 0.f;
	RepathTimer = 0.f;
	SetState(ETTHunterState::Pursue);
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

void ATTHunter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (State == ETTHunterState::Dormant)
	{
		return;
	}

	StateTime += DeltaSeconds;
	SpeechCooldown -= DeltaSeconds;
	UpdatePerception(DeltaSeconds);

	ATTPlayerCharacter* Player = GetPlayer();

	switch (State)
	{
	case ETTHunterState::Patrol:
		if (FollowPath(DeltaSeconds, HunterTuning::PatrolSpeed))
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
			StartInvestigate(SuspiciousLocation);
		}
		break;

	case ETTHunterState::Investigate:
		if (bSawPlayerThisFrame && FVector::Dist2D(LastKnown, InvestigateLocation) > 250.f && StateTime > 0.5f)
		{
			StartInvestigate(LastKnown);
		}
		else if (FollowPath(DeltaSeconds, HunterTuning::InvestigateSpeed))
		{
			StartSearch(InvestigateLocation, 14.f);
		}
		break;

	case ETTHunterState::Search:
		SearchTimeLeft -= DeltaSeconds;
		if (FollowPath(DeltaSeconds, HunterTuning::SearchSpeed))
		{
			if (ATTLocker* Locker = CurrentTarget.Locker.Get())
			{
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
		if (FollowPath(DeltaSeconds, Locker->IsOccupied() && TimeSinceSeen < 2.f ? 330.f : HunterTuning::SearchSpeed))
		{
			FaceTowards(Locker->GetActorLocation(), DeltaSeconds, 8.f);
			LockerTimer += DeltaSeconds;
			if (LockerTimer > 0.8f)
			{
				Locker->FlapDoor();
				if (Locker->IsOccupied())
				{
					Say(Locker->GetUses() >= 3 ? TEXT("Você sempre escolhe esse armário.") : TEXT("Acabou."));
					Capture();
				}
				else
				{
					TargetLocker.Reset();
					LookDuration = 1.2f;
					AfterLook = ETTHunterState::Search;
					SetState(ETTHunterState::LookAround);
				}
			}
		}
		break;
	}

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
				Say(TEXT("Acabou."));
				Capture();
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

		const bool bArrived = FollowPath(DeltaSeconds, HunterTuning::PursueSpeed);
		if (!bSawPlayerThisFrame && (bArrived || LostSightTime > 4.f))
		{
			++TimesLostJoe;
			Awareness = 0.6f;
			Say(TEXT("Eu sei que você está aí."), 0.5f);
			StartSearch(LastKnown, 22.f);
		}
		break;
	}

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
