#include "TTCompanion.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TTAudio.h"
#include "TTCompanionMind.h"
#include "TTGameMode.h"
#include "TTHospital.h"
#include "TTHunter.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"
#include "TTShapes.h"

namespace tc = thug::companion;

namespace
{
	enum EGoal : int32
	{
		GStayHidden = 0,
		GSearchResources = 1,
		GHelpJoe = 2,
		GShareRoute = 3,
		GEscapeAlone = 4,
		GUseJoeAsDistraction = 5
	};

	const TCHAR* GoalName(int32 Goal)
	{
		static const TCHAR* Names[] = { TEXT("StayHidden"), TEXT("SearchResources"), TEXT("HelpJoe"), TEXT("ShareRoute"), TEXT("EscapeAlone"), TEXT("UseJoeAsDistraction") };
		return (Goal >= 0 && Goal < 6) ? Names[Goal] : TEXT("?");
	}

	std::string ToStd(const FString& S)
	{
		return std::string(TCHAR_TO_UTF8(*S));
	}

	/**
	 * Authored starting profiles (0..100). The GDD describes the personalities (§5)
	 * but does not fix numbers: these are balance hypotheses, to be tuned in playtests.
	 */
	tc::Traits ProfileFor(ETTCompanionId Id)
	{
		tc::Traits T{};
		switch (Id)
		{
		case ETTCompanionId::Harley:      T = { 70, 35, 40, 75, 40, 85, 0, false }; break;
		case ETTCompanionId::Bartholomeu: T = { 55, 70, 60, 50, 65, 40, 0, false }; break;
		case ETTCompanionId::Jack:        T = { 35, 40, 30, 35, 70, 30, 1, false }; break;
		}
		return T;
	}

	tc::Difficulty ToCore(ETTDifficulty D)
	{
		switch (D)
		{
		case ETTDifficulty::Easy: return tc::Difficulty::Easy;
		case ETTDifficulty::Hard: return tc::Difficulty::Hard;
		case ETTDifficulty::Punitive: return tc::Difficulty::Punitive;
		default: return tc::Difficulty::Normal;
		}
	}

	namespace Speed
	{
		constexpr float Walk = 200.f;
		constexpr float Hurry = 320.f;
		constexpr float Flee = 380.f;
	}
}

ATTCompanion::ATTCompanion()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(32.f, 90.f);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 480.f, 0.f);
	Move->MaxWalkSpeed = Speed::Walk;
}

ATTCompanion::~ATTCompanion() = default;

void ATTCompanion::Setup(ETTCompanionId InId, ETTDifficulty InDifficulty)
{
	Id = InId;
	Difficulty = InDifficulty;
	Mind = MakeShared<FTTCompanionMind>(static_cast<tc::Character>(static_cast<int32>(Id)), ToCore(Difficulty), ProfileFor(Id),
		static_cast<uint64>(FMath::Rand()) + 17ull);

	// What each of them knows when the night starts (GDD §6 "Knowledge").
	const double T = Now();
	switch (Id)
	{
	case ETTCompanionId::Harley:
		Mind->Core.LearnRoute("medkit", T);
		break;
	case ETTCompanionId::Bartholomeu:
		Mind->Core.LearnRoute("keycard", T);
		break;
	case ETTCompanionId::Jack:
		Mind->Core.LearnRoute("keycard", T);
		Mind->Core.LearnRoute("window", T);
		bHasCrowbar = true;
		break;
	}
	if (!Body)
	{
		BuildVisual();
	}
}

void ATTCompanion::Activate()
{
	if (State == ETTCompanionState::Dormant)
	{
		SetState(ETTCompanionState::Idle);
		DecisionTimer = FMath::FRandRange(2.f, 5.f);
	}
}

void ATTCompanion::BuildVisual()
{
	USceneComponent* Root = GetCapsuleComponent();
	FLinearColor Coat(0.2f, 0.2f, 0.2f);
	FLinearColor Legs(0.08f, 0.08f, 0.1f);
	const FLinearColor Skin(0.55f, 0.42f, 0.34f);
	switch (Id)
	{
	case ETTCompanionId::Harley: Coat = FLinearColor(0.45f, 0.04f, 0.05f); Legs = FLinearColor(0.05f, 0.06f, 0.18f); break;
	case ETTCompanionId::Bartholomeu: Coat = FLinearColor(0.28f, 0.18f, 0.1f); break;
	case ETTCompanionId::Jack: Coat = FLinearColor(0.32f, 0.33f, 0.33f); break;
	}
	TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, -11.f, -48.f), FVector(18.f, 18.f, 86.f), Legs, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 11.f, -48.f), FVector(18.f, 18.f, 86.f), Legs, FRotator::ZeroRotator, false);
	Body = TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 28.f), FVector(30.f, 48.f, 70.f), Coat, FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(2.f, 0.f, 76.f), FVector(24.f, 22.f, 27.f), Skin, FRotator::ZeroRotator, false);
	if (Id == ETTCompanionId::Harley)
	{
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(-6.f, -15.f, 72.f), FVector(7.f, 7.f, 26.f), FLinearColor(0.85f, 0.75f, 0.4f), FRotator(0.f, 0.f, -25.f), false);
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(-6.f, 15.f, 72.f), FVector(7.f, 7.f, 26.f), FLinearColor(0.85f, 0.75f, 0.4f), FRotator(0.f, 0.f, 25.f), false);
	}
	else if (Id == ETTCompanionId::Jack)
	{
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 0.f, 93.f), FVector(30.f, 30.f, 5.f), FLinearColor(0.1f, 0.1f, 0.1f), FRotator::ZeroRotator, false);
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 0.f, 101.f), FVector(20.f, 20.f, 14.f), FLinearColor(0.1f, 0.1f, 0.1f), FRotator::ZeroRotator, false);
	}
}

// ---------------------------------------------------------------------------
// Accessors

ATTHospital* ATTCompanion::GetHospital() const
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	return GM ? GM->GetHospital() : nullptr;
}

ATTHunter* ATTCompanion::GetHunter() const
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	return GM ? GM->GetHunter() : nullptr;
}

ATTPlayerCharacter* ATTCompanion::GetJoe() const
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	return GM ? GM->GetPlayer() : nullptr;
}

double ATTCompanion::Now() const
{
	return GetWorld()->GetTimeSeconds();
}

FString ATTCompanion::NextEventId(const TCHAR* Kind)
{
	return FString::Printf(TEXT("%s_%s_%d"), *GetDisplayName(), Kind, ++EventCounter);
}

FString ATTCompanion::GetDisplayName() const
{
	switch (Id)
	{
	case ETTCompanionId::Harley: return TEXT("Harley");
	case ETTCompanionId::Bartholomeu: return TEXT("Bartholomeu");
	case ETTCompanionId::Jack: return TEXT("Jack");
	}
	return TEXT("?");
}

FString ATTCompanion::GetStateName() const
{
	if (const UEnum* Enum = StaticEnum<ETTCompanionState>())
	{
		return Enum->GetNameStringByValue(static_cast<int64>(State));
	}
	return FString();
}

float ATTCompanion::GetTrust() const { return Mind ? static_cast<float>(Mind->Core.CurrentTraits().trust) : 0.f; }
float ATTCompanion::GetLoyalty() const { return Mind ? static_cast<float>(Mind->Core.CurrentTraits().loyalty) : 0.f; }
float ATTCompanion::GetFear() const { return Mind ? static_cast<float>(Mind->Core.CurrentTraits().fear) : 0.f; }

FString ATTCompanion::GetTraitSummary() const
{
	if (!Mind)
	{
		return FString();
	}
	const tc::Traits& T = Mind->Core.CurrentTraits();
	return FString::Printf(TEXT("trust %.0f fear %.0f stress %.0f loyalty %.0f self %.0f rel %.0f ammo %d"),
		T.trust, T.fear, T.stress, T.loyalty, T.selfPreservation, T.relationship, T.ammunition);
}

float ATTCompanion::GetVisibility() const
{
	float Light = 0.f;
	if (const ATTHospital* Hospital = GetHospital())
	{
		Light = Hospital->GetLightLevelAt(GetActorLocation() + FVector(0.f, 0.f, 30.f), this);
	}
	const bool bRunning = GetVelocity().Size2D() > 260.f;
	return FMath::Clamp((0.35f + 0.65f * Light) * (bRunning ? 1.2f : 1.f), 0.f, 1.f);
}

void ATTCompanion::SetState(ETTCompanionState NewState)
{
	if (State != NewState)
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] %s %s -> %s"), *GetDisplayName(),
			*StaticEnum<ETTCompanionState>()->GetNameStringByValue(static_cast<int64>(State)),
			*StaticEnum<ETTCompanionState>()->GetNameStringByValue(static_cast<int64>(NewState)));
	}
	State = NewState;
	StateTime = 0.f;
}

void ATTCompanion::Say(const FString& Line, float Duration)
{
	// Joe only hears what is said near him.
	const ATTPlayerCharacter* Joe = GetJoe();
	if (!Joe || FVector::Dist(Joe->GetActorLocation(), GetActorLocation()) > 1600.f)
	{
		return;
	}
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->ShowSubtitle(FString::Printf(TEXT("%s: %s"), *GetDisplayName(), *Line), Duration);
	}
}

void ATTCompanion::ApplyInfluence(float Trust, float Fear, float Stress, float Loyalty, float SelfPreservation, float Relationship, int32 Ammo, const TCHAR* Kind)
{
	if (!Mind)
	{
		return;
	}
	tc::Influence D;
	D.trust = Trust;
	D.fear = Fear;
	D.stress = Stress;
	D.loyalty = Loyalty;
	D.selfPreservation = SelfPreservation;
	D.relationship = Relationship;
	D.ammunition = Ammo;
	try
	{
		Mind->Core.ApplyInfluence(D, ToStd(NextEventId(Kind)), Now());
	}
	catch (...)
	{
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] %s influence '%s' -> %s"), *GetDisplayName(), Kind, *GetTraitSummary());
}

// ---------------------------------------------------------------------------
// Perception

bool ATTCompanion::CanSee(const AActor* Target, const FVector& Point, float Range) const
{
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 70.f);
	if (FVector::Dist(Eye, Point) > Range)
	{
		return false;
	}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTCompanionLook), false, this);
	if (Target)
	{
		Query.AddIgnoredActor(Target);
	}
	return !GetWorld()->LineTraceTestByChannel(Eye, Point, ECC_Visibility, Query);
}

void ATTCompanion::UpdatePerception(float DeltaSeconds)
{
	PerceptionTimer -= DeltaSeconds;
	if (PerceptionTimer > 0.f)
	{
		return;
	}
	PerceptionTimer = 0.3f;
	const double T = Now();

	bSeesHunter = false;
	if (const ATTHunter* Hunter = GetHunter())
	{
		const float D = FVector::Dist(Hunter->GetActorLocation(), GetActorLocation());
		// Sees him, or hears his heavy steps close by.
		bSeesHunter = (D < 700.f && Hunter->GetVelocity().Size2D() > 30.f) || (!IsInLocker() && CanSee(Hunter, Hunter->GetEyeLocation(), 1600.f));
		if (bSeesHunter)
		{
			HunterSeenAt = T;
			HunterSeenLocation = Hunter->GetActorLocation();
		}
	}

	const bool bSawJoeBefore = bSeesJoe;
	bSeesJoe = false;
	if (const ATTPlayerCharacter* Joe = GetJoe())
	{
		if (!Joe->IsHidden() && !IsInLocker() && CanSee(Joe, Joe->GetEyeLocation(), 1500.f))
		{
			bSeesJoe = true;
			JoeSeenAt = T;
			JoeSeenLocation = Joe->GetActorLocation();
			JoeSeenHidingIn.Reset();
		}
		else if (bSawJoeBefore && Joe->IsHidden() && Joe->GetHiddenLocker())
		{
			// Watched him get into a locker: something the Hunter could get out of them.
			JoeSeenHidingIn = Joe->GetHiddenLocker()->GetId();
		}
	}
}

// ---------------------------------------------------------------------------
// Decisions (CompanionCore)

void ATTCompanion::Decide()
{
	if (!Mind)
	{
		return;
	}
	ATTHospital* Hospital = GetHospital();
	ATTPlayerCharacter* Joe = GetJoe();
	const ATTHunter* Hunter = GetHunter();
	const double T = Now();

	tc::Situation S;
	S.hunterPerceivedNearby = T - HunterSeenAt < 8.0;
	S.joePerceivedNearby = (T - JoeSeenAt < 6.0) || (T - FollowRequestTime < 20.0);
	S.joeNeedsHelp = Joe && (Joe->GetInjury() >= 2 || Joe->IsBleeding() || Joe->IsCollapsed()
		|| (Hunter && Hunter->IsChasing()) || T - FollowRequestTime < 20.0);
	if (Joe && Hospital && S.joePerceivedNearby)
	{
		TArray<FIntPoint> Route;
		S.canReachJoe = Hospital->FindPath(Hospital->WorldToCell(GetActorLocation()), Hospital->WorldToCell(JoeSeenLocation), Route);
	}
	for (TActorIterator<ATTPickup> It(GetWorld()); It; ++It)
	{
		if (It->GetType() == ETTPickupType::Item && CanSee(*It, It->GetActorLocation() + FVector(0.f, 0.f, 40.f), 1200.f))
		{
			S.resourcesKnownNearby = true;
			break;
		}
	}
	for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It)
	{
		if (Hospital && (bHasCrowbar || It->IsOpen()))
		{
			TArray<FIntPoint> Route;
			if (Hospital->FindPath(Hospital->WorldToCell(GetActorLocation()), Hospital->WorldToCell(It->GetApproachLocation()), Route))
			{
				S.accessibleEscapeRoute = std::string("window");
			}
		}
	}

	tc::Option Choice;
	try
	{
		Choice = Mind->Core.Choose(S);
	}
	catch (...)
	{
		return;
	}
	const int32 Goal = static_cast<int32>(Choice.goal);
	UE_LOG(LogTemp, Log, TEXT("[TT] %s decides %s (hunter %d, joe %d, needsHelp %d, reach %d, resources %d, exit %d) %s"),
		*GetDisplayName(), GoalName(Goal), S.hunterPerceivedNearby ? 1 : 0, S.joePerceivedNearby ? 1 : 0, S.joeNeedsHelp ? 1 : 0,
		S.canReachJoe ? 1 : 0, S.resourcesKnownNearby ? 1 : 0, S.accessibleEscapeRoute ? 1 : 0, *GetTraitSummary());
	ExecuteGoal(Goal, Choice.routeId ? FString(UTF8_TO_TCHAR(Choice.routeId->c_str())) : FString());
}

void ATTCompanion::ExecuteGoal(int32 Goal, const FString& RouteId)
{
	switch (Goal)
	{
	case GStayHidden: StartHiding(); break;
	case GSearchResources: StartSearchResource(); break;
	case GHelpJoe: StartGoToJoe(false); break;
	case GShareRoute: StartGoToJoe(true); break;
	case GEscapeAlone: StartEscape(); break;
	case GUseJoeAsDistraction: StartBetrayal(); break;
	default: SetState(ETTCompanionState::Idle); break;
	}
}

void ATTCompanion::DebugForceGoal(int32 Goal)
{
	if (IsInLocker())
	{
		LeaveLocker();
	}
	ExecuteGoal(Goal, FString());
}

// ---------------------------------------------------------------------------
// Goals

void ATTCompanion::StartHiding()
{
	ATTHospital* Hospital = GetHospital();
	ATTLocker* Best = nullptr;
	float BestDist = 1800.f;
	if (Hospital)
	{
		for (const TObjectPtr<ATTLocker>& L : Hospital->GetLockers())
		{
			if (!L || L->IsOccupied())
			{
				continue;
			}
			// Away from where the Hunter was last seen.
			const float D = FVector::Dist2D(L->GetActorLocation(), GetActorLocation());
			const float FromHunter = FVector::Dist2D(L->GetActorLocation(), HunterSeenLocation);
			if (D < BestDist && (Now() - HunterSeenAt > 8.0 || FromHunter > 900.f))
			{
				BestDist = D;
				Best = L.Get();
			}
		}
	}
	if (Best && Mover.Build(this, Hospital, Best->GetFrontLocation(), GetJoe()))
	{
		TargetLocker = Best;
		SetState(ETTCompanionState::Hiding);
	}
	else
	{
		SetState(ETTCompanionState::Idle);
		DecisionTimer = FMath::FRandRange(6.f, 10.f);
	}
}

void ATTCompanion::EnterLocker(ATTLocker* Locker)
{
	if (!Locker || Locker->IsOccupied())
	{
		SetState(ETTCompanionState::Idle);
		return;
	}
	HiddenIn = Locker;
	Locker->SetOccupied(true, this);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->StopMovementImmediately();
	SetActorLocation(Locker->GetInsideLocation() + FVector(0.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->ReportNoise(Locker->GetActorLocation(), ETTNoise::Locker, this);
	}
	SetState(ETTCompanionState::Hidden);
	DecisionTimer = FMath::FRandRange(25.f, 40.f);
}

void ATTCompanion::LeaveLocker()
{
	ATTLocker* Locker = HiddenIn.Get();
	HiddenIn.Reset();
	if (!Locker)
	{
		return;
	}
	Locker->SetOccupied(false, nullptr);
	SetActorLocation(Locker->GetFrontLocation() + FVector(0.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->ReportNoise(Locker->GetActorLocation(), ETTNoise::Locker, this);
	}
	SetState(ETTCompanionState::Idle);
}

void ATTCompanion::StartFlee()
{
	ATTHospital* Hospital = GetHospital();
	if (!Hospital)
	{
		return;
	}
	// Farthest reachable spot from the Hunter within a few cells.
	const FIntPoint Here = Hospital->WorldToCell(GetActorLocation());
	FVector Best = GetActorLocation();
	float BestScore = -1.f;
	for (const FIntPoint& C : Hospital->GetWalkableCellsNear(Here, 5))
	{
		const FVector P = Hospital->CellToWorld(C);
		const float Score = FVector::Dist2D(P, HunterSeenLocation) - 0.3f * FVector::Dist2D(P, GetActorLocation());
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = P;
		}
	}
	Mover.Build(this, Hospital, Best, GetJoe());
	SetState(ETTCompanionState::Fleeing);
}

void ATTCompanion::StartSearchResource()
{
	ATTPickup* Best = nullptr;
	float BestDist = 1200.f;
	for (TActorIterator<ATTPickup> It(GetWorld()); It; ++It)
	{
		const float D = FVector::Dist(It->GetActorLocation(), GetActorLocation());
		if (It->GetType() == ETTPickupType::Item && D < BestDist && CanSee(*It, It->GetActorLocation() + FVector(0.f, 0.f, 40.f), 1200.f))
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best && Mover.Build(this, GetHospital(), Best->GetActorLocation(), GetJoe()))
	{
		TargetPickup = Best;
		SetState(ETTCompanionState::Searching);
	}
	else
	{
		SetState(ETTCompanionState::Idle);
		DecisionTimer = 5.f;
	}
}

void ATTCompanion::StartGoToJoe(bool bToShareRoute)
{
	bShareRouteOnArrival = bToShareRoute;
	if (Mover.Build(this, GetHospital(), JoeSeenLocation, GetJoe()))
	{
		SetState(ETTCompanionState::GoingToJoe);
	}
	else
	{
		SetState(ETTCompanionState::Idle);
		DecisionTimer = 5.f;
	}
}

void ATTCompanion::StartEscape()
{
	for (TActorIterator<ATTWindow> It(GetWorld()); It; ++It)
	{
		if (Mover.Build(this, GetHospital(), It->GetApproachLocation(), GetJoe()))
		{
			TargetWindow = *It;
			SetState(ETTCompanionState::Prying);
			return;
		}
	}
	SetState(ETTCompanionState::Idle);
}

void ATTCompanion::StartBetrayal()
{
	if (Mover.Build(this, GetHospital(), JoeSeenLocation, GetJoe()))
	{
		SetState(ETTCompanionState::Betraying);
	}
	else
	{
		StartFlee();
	}
}

FString ATTCompanion::NextUnsharedRoute() const
{
	if (!Mind)
	{
		return FString();
	}
	for (const std::string& R : Mind->Core.KnownRoutes())
	{
		const FString Route = UTF8_TO_TCHAR(R.c_str());
		if (!SharedRoutes.Contains(Route))
		{
			return Route;
		}
	}
	return FString();
}

void ATTCompanion::ShareRouteWithJoe()
{
	const FString Route = NextUnsharedRoute();
	if (Route.IsEmpty())
	{
		return;
	}
	SharedRoutes.Add(Route);
	ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	UE_LOG(LogTemp, Log, TEXT("[TT] %s shares route '%s' with Joe"), *GetDisplayName(), *Route);
	if (Route == TEXT("keycard"))
	{
		Say(TEXT("O cartão do portão? Vi numa mesa, na enfermaria do canto sudoeste."), 6.f);
		if (GM)
		{
			GM->SetObjective(TEXT("O cartão do portão está na enfermaria do canto sudoeste."));
		}
	}
	else if (Route == TEXT("window"))
	{
		Say(TEXT("Tem uma janela pregada na enfermaria sudoeste. Com um pé de cabra dá pra sair pro prédio do lado."), 7.f);
		if (GM)
		{
			GM->SetObjective(TEXT("Janela pregada na enfermaria sudoeste: precisa de um pé de cabra."));
		}
	}
	else if (Route == TEXT("medkit"))
	{
		Say(TEXT("Tem um kit médico na sala de baixo, a do meio. Eu vi."), 5.f);
	}
}

// ---------------------------------------------------------------------------
// Joe

FString ATTCompanion::GetPrompt(const ATTPlayerCharacter* Joe) const
{
	if (!IsVisibleTarget())
	{
		return FString();
	}
	return FString::Printf(TEXT("[E] Falar com %s    [Z] Dar carregador    [X] %s"), *GetDisplayName(),
		State == ETTCompanionState::Following ? TEXT("Fica escondido") : TEXT("Vem comigo"));
}

void ATTCompanion::Talk(ATTPlayerCharacter* Joe)
{
	if (!Joe || !IsVisibleTarget() || LineCooldown > 0.f)
	{
		return;
	}
	LineCooldown = 2.5f;
	JoeSeenAt = Now();
	JoeSeenLocation = Joe->GetActorLocation();
	const FRotator Face = (Joe->GetActorLocation() - GetActorLocation()).Rotation();
	SetActorRotation(FRotator(0.f, Face.Yaw, 0.f));

	const float Trust = GetTrust();
	if (Trust >= 40.f && !NextUnsharedRoute().IsEmpty())
	{
		ShareRouteWithJoe();
	}
	else if (Joe->IsBleeding() && Id == ETTCompanionId::Harley)
	{
		Say(TEXT("Você tá sangrando. Não mente pra mim."));
	}
	else
	{
		switch (Id)
		{
		case ETTCompanionId::Harley:
			Say(Trust >= 50.f ? TEXT("Joe, olha pra mim. A gente precisa sair daqui.") : TEXT("Então é isso? Você vai me deixar?"));
			break;
		case ETTCompanionId::Bartholomeu:
			Say(Trust >= 50.f ? TEXT("A gente precisa sair daqui. Thomas morreu, Joe.") : TEXT("Não. Dessa vez eu vou sozinho."));
			break;
		case ETTCompanionId::Jack:
			Say(Trust >= 50.f ? TEXT("Ele não tá procurando a gente. Ele tá aprendendo.") : TEXT("Fala baixo. Ele ouve tudo."));
			break;
		}
	}

	// GDD §2.5: a tool left for Joe opens a route that might not exist in another run.
	if (bHasCrowbar && Trust >= 60.f && Joe->GetItemCount(ETTItem::Crowbar) == 0 && Joe->TryAddItem(ETTItem::Crowbar, 1))
	{
		bHasCrowbar = false;
		Say(TEXT("Toma. Pé de cabra. Se eu não sair, sai você."), 5.f);
		UE_LOG(LogTemp, Log, TEXT("[TT] %s gave Joe the crowbar"), *GetDisplayName());
	}
}

void ATTCompanion::RequestFollow(ATTPlayerCharacter* Joe)
{
	if (!Joe || !IsVisibleTarget())
	{
		return;
	}
	if (State == ETTCompanionState::Following)
	{
		Say(TEXT("Tá. Eu espero aqui."), 3.f);
		StartHiding();
		return;
	}
	FollowRequestTime = Now();
	JoeSeenAt = Now();
	JoeSeenLocation = Joe->GetActorLocation();
	if (GetTrust() >= 45.f)
	{
		Say(Id == ETTCompanionId::Harley ? TEXT("Eu vou com você.") : TEXT("Tá. Mas se ele aparecer, cada um por si."), 3.f);
		SetState(ETTCompanionState::Following);
	}
	else
	{
		Say(Id == ETTCompanionId::Jack ? TEXT("Pra quê? Pra ele pegar nós dois?") : TEXT("Não. Dessa vez eu vou sozinho."), 3.f);
	}
}

void ATTCompanion::OnReceivedAmmo()
{
	UE_LOG(LogTemp, Log, TEXT("[TT] %s received ammo -> %s"), *GetDisplayName(), *GetTraitSummary());
	switch (Id)
	{
	case ETTCompanionId::Harley: Say(TEXT("Tá. Mas fica perto de mim.")); break;
	case ETTCompanionId::Bartholomeu: Say(TEXT("Eu vou lembrar disso.")); break;
	case ETTCompanionId::Jack: Say(TEXT("Interessante. Obrigado, Joe.")); break;
	}
}

void ATTCompanion::OnSavedByJoe()
{
	if (!bChased)
	{
		return;
	}
	bSavedDuringChase = true;
	// Authored influence (hypothesis): being saved builds trust and loyalty.
	ApplyInfluence(15.f, 0.f, -5.f, 10.f, 0.f, 5.f, 0, TEXT("saved_by_joe"));
	Say(Id == ETTCompanionId::Harley ? TEXT("Eu sabia que você não ia me deixar.") : TEXT("Te devo essa."));
}

// ---------------------------------------------------------------------------
// Hunter

void ATTCompanion::OnChasedByHunter(bool bStarted)
{
	if (bStarted)
	{
		if (bChased)
		{
			return;
		}
		bChased = true;
		bSavedDuringChase = false;
		const ATTPlayerCharacter* Joe = GetJoe();
		bJoeWitnessedChase = Joe && !Joe->IsHidden() && CanSee(Joe, Joe->GetEyeLocation(), 1500.f);
		Say(TEXT("Joe! JOE!"), 2.f);
		if (IsInLocker())
		{
			return;
		}
		StartFlee();
		return;
	}
	if (!bChased)
	{
		return;
	}
	bChased = false;
	// GDD §51: Joe saw it happen and did nothing.
	if (bJoeWitnessedChase && !bSavedDuringChase && Mind)
	{
		bool bApplied = false;
		try
		{
			bApplied = Mind->Core.ApplyExperience(tc::Experience::JoeAbandonedCompanion, ToStd(NextEventId(TEXT("abandoned"))), Now());
		}
		catch (...)
		{
		}
		if (bApplied)
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] %s felt abandoned by Joe -> %s"), *GetDisplayName(), *GetTraitSummary());
		}
	}
}

bool ATTCompanion::TakenByHunter(FVector& OutJoeLocation, FString& OutHideSpotId)
{
	if (IsInLocker())
	{
		if (ATTLocker* L = HiddenIn.Get())
		{
			L->SetOccupied(false, nullptr);
		}
		HiddenIn.Reset();
	}
	Say(TEXT("Não! NÃO—"), 2.5f);
	TTAudio::PlayAt(this, ETTSfx::Punch, GetActorLocation(), 1.f);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->StopMovementImmediately();
	SetState(ETTCompanionState::Taken);

	OutHideSpotId = JoeSeenHidingIn;
	const bool bKnowsJoe = Now() - JoeSeenAt < 90.0;
	if (bKnowsJoe)
	{
		OutJoeLocation = JoeSeenLocation;
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] %s was taken by the Hunter (knew Joe's position: %d, hiding spot: %s)"),
		*GetDisplayName(), bKnowsJoe ? 1 : 0, OutHideSpotId.IsEmpty() ? TEXT("-") : *OutHideSpotId);
	return bKnowsJoe || !OutHideSpotId.IsEmpty();
}

void ATTCompanion::OnFriendTaken(const ATTCompanion* Friend)
{
	if (IsActive() && Friend != this)
	{
		ApplyInfluence(0.f, 15.f, 10.f, 0.f, 5.f, 0.f, 0, TEXT("friend_taken"));
	}
}

// ---------------------------------------------------------------------------
// Tick

void ATTCompanion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsActive())
	{
		return;
	}
	StateTime += DeltaSeconds;
	DecisionTimer -= DeltaSeconds;
	LineCooldown -= DeltaSeconds;
	if (Mind)
	{
		try
		{
			// The core keeps its own clock for event ordering.
			Mind->Core.SetInjured(Mind->Core.CurrentTraits().injured, Now());
		}
		catch (...)
		{
		}
	}

	UpdatePerception(DeltaSeconds);
	ATTPlayerCharacter* Joe = GetJoe();
	ATTHospital* Hospital = GetHospital();
	const ATTHunter* Hunter = GetHunter();

	// Panic overrides plans: the Hunter is right there.
	const bool bHunterClose = bSeesHunter && FVector::Dist(HunterSeenLocation, GetActorLocation()) < 1100.f;
	if (bHunterClose && !IsInLocker() && State != ETTCompanionState::Fleeing && State != ETTCompanionState::Betraying
		&& State != ETTCompanionState::Following && !(State == ETTCompanionState::Prying && StateTime > 4.f))
	{
		StartFlee();
	}

	switch (State)
	{
	case ETTCompanionState::Idle:
		if (DecisionTimer <= 0.f)
		{
			DecisionTimer = FMath::FRandRange(6.f, 10.f);
			Decide();
		}
		break;

	case ETTCompanionState::Hiding:
	{
		ATTLocker* Locker = TargetLocker.Get();
		if (!Locker || Locker->IsOccupied())
		{
			StartHiding();
			break;
		}
		if (Mover.Follow(this, Hospital, DeltaSeconds, bSeesHunter ? Speed::Flee : Speed::Hurry))
		{
			EnterLocker(Locker);
		}
		break;
	}

	case ETTCompanionState::Hidden:
		if (DecisionTimer <= 0.f)
		{
			// Only comes out when the Hunter has not been heard for a while.
			if (Hunter && FVector::Dist(Hunter->GetActorLocation(), GetActorLocation()) > 1500.f)
			{
				LeaveLocker();
				DecisionTimer = 1.f;
			}
			else
			{
				DecisionTimer = 10.f;
			}
		}
		break;

	case ETTCompanionState::Searching:
	{
		ATTPickup* Pickup = TargetPickup.Get();
		if (!Pickup)
		{
			SetState(ETTCompanionState::Idle);
			break;
		}
		if (Mover.Follow(this, Hospital, DeltaSeconds, Speed::Walk) || FVector::Dist2D(Pickup->GetActorLocation(), GetActorLocation()) < 120.f)
		{
			// GDD §49: they find, keep or steal things.
			const ETTItem Item = Pickup->GetItem();
			if (Item == ETTItem::Magazine)
			{
				ApplyInfluence(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1, TEXT("found_ammo"));
			}
			else if (Item == ETTItem::Crowbar)
			{
				bHasCrowbar = true;
			}
			UE_LOG(LogTemp, Log, TEXT("[TT] %s took a %s"), *GetDisplayName(), *TTItemName(Item));
			Pickup->Destroy();
			SetState(ETTCompanionState::Idle);
			DecisionTimer = 3.f;
		}
		break;
	}

	case ETTCompanionState::GoingToJoe:
	{
		if (Joe && bSeesJoe && StateTime > 1.f && FMath::Fmod(StateTime, 1.f) < DeltaSeconds)
		{
			Mover.Build(this, Hospital, Joe->GetActorLocation(), Joe);
		}
		const bool bArrived = Mover.Follow(this, Hospital, DeltaSeconds, Speed::Hurry);
		const bool bNear = Joe && FVector::Dist2D(Joe->GetActorLocation(), GetActorLocation()) < 280.f;
		if (bNear)
		{
			if (bShareRouteOnArrival)
			{
				ShareRouteWithJoe();
			}
			else if (Joe->IsBleeding() && !bGaveAid)
			{
				bGaveAid = true;
				Joe->ReceiveAid(GetDisplayName());
				Say(Id == ETTCompanionId::Harley ? TEXT("Você tá sangrando. Fica quieto, deixa eu ver.") : TEXT("Segura isso aqui. Aperta forte."));
			}
			SetState(ETTCompanionState::Following);
		}
		else if (bArrived || StateTime > 25.f)
		{
			SetState(ETTCompanionState::Idle);
			DecisionTimer = 4.f;
		}
		break;
	}

	case ETTCompanionState::Following:
	{
		if (!Joe || StateTime > 45.f)
		{
			SetState(ETTCompanionState::Idle);
			break;
		}
		if (Joe->IsHidden())
		{
			StartHiding();
			break;
		}
		const float D = FVector::Dist2D(Joe->GetActorLocation(), GetActorLocation());
		if (D > 420.f && (!Mover.HasPath() || FMath::Fmod(StateTime, 0.7f) < DeltaSeconds))
		{
			Mover.Build(this, Hospital, Joe->GetActorLocation() - (Joe->GetActorLocation() - GetActorLocation()).GetSafeNormal() * 250.f, Joe);
		}
		if (D > 300.f)
		{
			Mover.Follow(this, Hospital, DeltaSeconds, D > 900.f ? Speed::Flee : Speed::Hurry);
		}
		// GDD §49: they can save Joe. A shot buys him a second.
		ATTHunter* MutableHunter = GetHunter();
		if (MutableHunter && MutableHunter->IsChasing() && Mind && Mind->Core.CurrentTraits().ammunition > 0
			&& Now() - LastHelpShot > 6.0 && CanSee(MutableHunter, MutableHunter->GetEyeLocation(), 1500.f))
		{
			LastHelpShot = Now();
			ApplyInfluence(0.f, 5.f, 5.f, 0.f, 0.f, 0.f, -1, TEXT("shot_for_joe"));
			TTAudio::PlayAt(this, ETTSfx::Gunshot, GetActorLocation(), 0.9f);
			MutableHunter->Stun(1.2f, false);
			if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
			{
				GM->ReportNoise(GetActorLocation(), ETTNoise::Gunshot, this);
			}
			Say(TEXT("Corre, Joe!"), 2.f);
		}
		break;
	}

	case ETTCompanionState::Fleeing:
		if (Mover.Follow(this, Hospital, DeltaSeconds, Speed::Flee) || StateTime > 12.f)
		{
			StartHiding();
		}
		break;

	case ETTCompanionState::Prying:
	{
		ATTWindow* Window = TargetWindow.Get();
		if (!Window)
		{
			SetState(ETTCompanionState::Idle);
			break;
		}
		const bool bAtWindow = FVector::Dist2D(Window->GetApproachLocation(), GetActorLocation()) < 120.f;
		if (!bAtWindow)
		{
			if (Mover.Follow(this, Hospital, DeltaSeconds, Speed::Hurry) && FVector::Dist2D(Window->GetApproachLocation(), GetActorLocation()) > 200.f)
			{
				SetState(ETTCompanionState::Idle);
			}
			break;
		}
		SetActorRotation(FRotator(0.f, (Window->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f));
		if (Window->IsOpen() || Window->AddPry(DeltaSeconds, this))
		{
			UE_LOG(LogTemp, Log, TEXT("[TT] %s escaped through the window"), *GetDisplayName());
			Say(TEXT("Desculpa, Joe."), 2.f);
			SetActorHiddenInGame(true);
			SetActorEnableCollision(false);
			SetState(ETTCompanionState::Escaped);
			if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
			{
				if (Joe && FVector::Dist(Joe->GetActorLocation(), GetActorLocation()) < 2600.f)
				{
					GM->ShowMessage(TEXT("Madeira sendo arrancada... alguém fugiu pela janela da enfermaria sudoeste."), 5.f);
				}
			}
		}
		break;
	}

	case ETTCompanionState::Betraying:
	{
		const bool bArrived = Mover.Follow(this, Hospital, DeltaSeconds, Speed::Hurry);
		if (Joe && (bArrived || FVector::Dist2D(Joe->GetActorLocation(), GetActorLocation()) < 500.f || StateTime > 15.f))
		{
			// GDD §51: uses Joe as a distraction, then runs the other way.
			Say(Id == ETTCompanionId::Bartholomeu ? TEXT("Desculpa, Joe. Eu quero viver. ELE TÁ AQUI!") : TEXT("ELE TÁ AQUI! O JOE TÁ AQUI!"), 4.f);
			UE_LOG(LogTemp, Log, TEXT("[TT] %s betrayed Joe"), *GetDisplayName());
			if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
			{
				GM->ReportNoise(Joe->GetActorLocation() + FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 0.f), ETTNoise::Glass, this);
			}
			HunterSeenLocation = Joe->GetActorLocation();
			HunterSeenAt = Now();
			StartFlee();
		}
		break;
	}

	default:
		break;
	}
}
