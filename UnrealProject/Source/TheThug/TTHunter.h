#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TTHunter.generated.h"

class ATTHospital;
class ATTLocker;
class ATTPlayerCharacter;

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
	Capturing
};

/**
 * The Hunter (GDD §7-§20). Perceives with sight and hearing only, remembers locker use,
 * can be fooled, and never reads Joe's exact position unless it can actually see him.
 */
UCLASS()
class THETHUG_API ATTHunter : public ACharacter
{
	GENERATED_BODY()

public:
	ATTHunter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void Activate();
	void Freeze();

	void OnNoiseHeard(const FVector& Location, float Loudness);
	void OnPlayerHid(ATTLocker* Locker);

	bool IsChasing() const { return State == ETTHunterState::Pursue; }
	ETTHunterState GetState() const { return State; }
	float GetAwareness() const { return Awareness; }
	FString GetStateName() const;
	FVector GetEyeLocation() const { return GetActorLocation() + FVector(0.f, 0.f, 78.f); }

private:
	struct FSearchTarget
	{
		FVector Location = FVector::ZeroVector;
		TWeakObjectPtr<ATTLocker> Locker;
	};

	ATTHospital* GetHospital() const;
	ATTPlayerCharacter* GetPlayer() const;

	void BuildVisual();
	void SetState(ETTHunterState NewState);
	void Say(const FString& Line, float Chance = 1.f);

	bool CanSeePlayer(float& OutDistance, float& OutVisibility) const;
	void UpdatePerception(float DeltaSeconds);

	bool MoveTo(const FVector& Goal);
	/** Returns true when the end of the path is reached. */
	bool FollowPath(float DeltaSeconds, float Speed);
	void FaceTowards(const FVector& Target, float DeltaSeconds, float Rate = 6.f);
	void UpdateFootsteps(float DeltaSeconds);

	void StartPatrol();
	void StartInvestigate(const FVector& Location);
	void StartSearch(const FVector& Center, float Duration);
	void NextSearchTarget();
	void StartCheckLocker(ATTLocker* Locker);
	void StartPursue();
	void Capture();

	ETTHunterState State = ETTHunterState::Dormant;
	float StateTime = 0.f;
	float Awareness = 0.f;

	TArray<FVector> Path;
	TArray<FIntPoint> PathCells;
	int32 PathIndex = 0;
	float DoorWait = 0.f;
	float StuckTime = 0.f;

	FVector LastKnown = FVector::ZeroVector;
	FVector LastKnownVelocity = FVector::ZeroVector;
	FVector InvestigateLocation = FVector::ZeroVector;
	FVector SuspiciousLocation = FVector::ZeroVector;
	float LostSightTime = 0.f;
	float RepathTimer = 0.f;
	bool bSawPlayerThisFrame = false;
	float TimeSinceSeen = 100.f;

	FVector SearchCenter = FVector::ZeroVector;
	float SearchTimeLeft = 0.f;
	TArray<FSearchTarget> SearchQueue;
	FSearchTarget CurrentTarget;

	TWeakObjectPtr<ATTLocker> TargetLocker;
	float LockerTimer = 0.f;

	int32 PatrolIndex = -1;
	float LookDuration = 0.f;
	float LookBaseYaw = 0.f;
	ETTHunterState AfterLook = ETTHunterState::Patrol;

	float StepDistance = 0.f;
	float SpeechCooldown = 0.f;
	int32 TimesLostJoe = 0;
};
