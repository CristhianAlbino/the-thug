#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TTTypes.h"
#include "TTHunter.generated.h"

class ATTCompanion;
class ATTDoor;
class ATTHospital;
class ATTLocker;
class ATTPlayerCharacter;
struct FTTHunterMind;

UENUM()
enum class ETTHunterState : uint8
{
	Dormant,
	Patrol,
	LookAround,
	Suspicious,
	Investigate,
	Search,
	Pursue,
	CheckLocker,
	Ambush,
	SetTrap,
	Stunned,
	PursueCompanion,
	Interrogate,
	Capturing
};

/**
 * The Hunter (GDD §7-§20).
 * Memory and hearing come from the engine-independent cores in /Tools:
 *  - thug::hunter::HunterCore        routes, hiding spots, decay, difficulty tuning
 *  - thug::perception::PerceptionCore approximate hearing with an uncertainty radius
 *  - thug::perception::EvidenceMemory doors he remembers closed and later finds open
 * This actor turns those intentions into movement, traps and ambushes.
 * Golden rule: he only knows what his eyes, ears and inspected evidence told him.
 */
UCLASS()
class THETHUG_API ATTHunter : public ACharacter
{
	GENERATED_BODY()

public:
	ATTHunter();
	virtual ~ATTHunter() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Configure(ETTDifficulty InDifficulty);
	void Activate();
	void Freeze();

	void OnNoiseHeard(const FVector& Location, ETTNoise Kind);
	void OnPlayerHid(ATTLocker* Locker);
	void OnPlayerLeftLocker(ATTLocker* Locker);
	/** Shot or caught in a blast: stops him for a moment (GDD §37-38). It never kills him. */
	void Stun(float Seconds, bool bExplosion);

	bool IsChasing() const { return State == ETTHunterState::Pursue; }
	ETTHunterState GetState() const { return State; }
	float GetAwareness() const { return Awareness; }
	float GetAggression() const { return Aggression; }
	FString GetStateName() const;
	FString GetMemorySummary() const;
	FVector GetEyeLocation() const { return GetActorLocation() + FVector(0.f, 0.f, 78.f); }

	/** Test hook: feeds observations as if he had seen Joe use this door. */
	void DebugObserveRoute(const FIntPoint& DoorCell, int32 Times);
	void DebugSetAggression(float Value) { Aggression = Value; }

private:
	struct FSearchTarget
	{
		FVector Location = FVector::ZeroVector;
		TWeakObjectPtr<ATTLocker> Locker;
	};

	ATTHospital* GetHospital() const;
	ATTPlayerCharacter* GetPlayer() const;
	double Now() const;
	float Speed(float Base) const;

	void BuildVisual();
	void SetState(ETTHunterState NewState);
	void Say(const FString& Line, float Chance = 1.f);

	// Perception
	bool CanSeePlayer(float& OutDistance, float& OutVisibility) const;
	bool CanSeePoint(const FVector& Point, const AActor* Target) const;
	void UpdatePerception(float DeltaSeconds);
	void ObserveRoutesNear(const FIntPoint& PlayerCell);
	void InspectDoors();
	void RebaselineDoor(const ATTDoor* Door);

	// Memory queries (HunterCore)
	float HideSpotSuspicion(const ATTLocker* Locker) const;
	float RouteEvidence(const FIntPoint& DoorCell) const;
	void ObserveHideSpot(const ATTLocker* Locker, double Strength);

	// Movement
	bool MoveTo(const FVector& Goal);
	bool FollowPath(float DeltaSeconds, float MoveSpeed);
	void FaceTowards(const FVector& Target, float DeltaSeconds, float Rate = 6.f);
	void UpdateFootsteps(float DeltaSeconds);
	void UpdateDoorClosing();

	// Behaviour
	void StartPatrol();
	void ResumePatrol();
	void StartInvestigate(const FVector& Location, float RadiusCm);
	void StartSearch(const FVector& Center, float Duration);
	void NextSearchTarget();
	void StartCheckLocker(ATTLocker* Locker);
	void StartPursue();
	void LosePursuit();
	bool TryStartTrap();
	bool TryStartAmbush();
	void Capture();
	/** Contact with Joe: capture, or (lower difficulties, Joe not yet badly hurt) a blow that injures him. */
	void ReachJoe();
	void ScanCompanions();
	void StartPursueCompanion(ATTCompanion* Companion);
	void TakeCompanion(ATTCompanion* Companion);

	TSharedPtr<FTTHunterMind> Mind;
	ETTDifficulty Difficulty = ETTDifficulty::Normal;
	float SpeedScale = 1.f;
	float SightScale = 1.f;
	int32 SearchTargets = 4;
	float SearchSeconds = 28.f;

	ETTHunterState State = ETTHunterState::Dormant;
	ETTHunterState PreviousState = ETTHunterState::Dormant;
	float StateTime = 0.f;
	float Awareness = 0.f;
	/** GDD §18. Rises when Joe escapes or fools him; makes him faster, more patient, more willing to set traps. */
	float Aggression = 0.f;

	TArray<FVector> Path;
	int32 PathIndex = 0;
	float DoorWait = 0.f;
	float StuckTime = 0.f;
	FIntPoint PendingCloseDoor = FIntPoint(-1, -1);

	FVector LastKnown = FVector::ZeroVector;
	FVector LastKnownVelocity = FVector::ZeroVector;
	FVector InvestigateLocation = FVector::ZeroVector;
	float InvestigateRadius = 400.f;
	FVector SuspiciousLocation = FVector::ZeroVector;
	bool bSuspiciousFaint = false;
	float LostSightTime = 0.f;
	float RepathTimer = 0.f;
	bool bSawPlayerThisFrame = false;
	float TimeSinceSeen = 100.f;
	float VisualReportTimer = 0.f;
	float DoorInspectTimer = 0.f;
	TMap<FIntPoint, double> RouteObservedAt;

	FVector SearchCenter = FVector::ZeroVector;
	float SearchTimeLeft = 0.f;
	TArray<FSearchTarget> SearchQueue;
	FSearchTarget CurrentTarget;

	TWeakObjectPtr<ATTLocker> TargetLocker;
	bool bLockerKnown = false;
	float LockerTimer = 0.f;

	FVector PatrolTarget = FVector::ZeroVector;
	float LookDuration = 0.f;
	float LookBaseYaw = 0.f;
	ETTHunterState AfterLook = ETTHunterState::Patrol;

	FIntPoint AmbushDoor = FIntPoint(-1, -1);
	FVector AmbushPoint = FVector::ZeroVector;
	bool bAmbushSettled = false;
	float AmbushTimeLeft = 0.f;
	float AmbushCooldown = 45.f;

	FIntPoint TrapDoor = FIntPoint(-1, -1);
	float TrapCooldown = 60.f;

	float StepDistance = 0.f;
	float SpeechCooldown = 0.f;
	int32 TimesLostJoe = 0;
	float StunTimeLeft = 0.f;
	TWeakObjectPtr<ATTCompanion> TargetCompanion;
	float CompanionScanTimer = 0.f;
	float CompanionLostTime = 0.f;
	FVector CompanionLastSeen = FVector::ZeroVector;
	bool bInterrogationInfo = false;
	FVector InterrogationLocation = FVector::ZeroVector;
	FString InterrogationHideSpot;
	float StrikeRecover = 0.f;
};
