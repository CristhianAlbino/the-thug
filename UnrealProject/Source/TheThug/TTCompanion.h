#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TTPath.h"
#include "TTTypes.h"
#include "TTCompanion.generated.h"

class ATTHospital;
class ATTHunter;
class ATTLocker;
class ATTPickup;
class ATTPlayerCharacter;
class ATTWindow;
struct FTTCompanionMind;

UENUM()
enum class ETTCompanionId : uint8
{
	Harley,
	Bartholomeu,
	Jack
};

UENUM()
enum class ETTCompanionState : uint8
{
	Dormant,
	Idle,
	Hiding,
	Hidden,
	Searching,
	GoingToJoe,
	Following,
	Fleeing,
	Prying,
	Betraying,
	Escaped,
	Taken
};

/**
 * Harley, Bartholomeu and Jack (GDD §5, §6, §49-51).
 * Each one perceives the world on their own, keeps their own knowledge, and lets
 * thug::companion::CompanionCore (in /Tools) pick what to do. This actor executes it.
 */
UCLASS()
class THETHUG_API ATTCompanion : public ACharacter
{
	GENERATED_BODY()

public:
	ATTCompanion();
	virtual ~ATTCompanion() override;

	void Setup(ETTCompanionId InId, ETTDifficulty Difficulty);
	void Activate();
	virtual void Tick(float DeltaSeconds) override;

	ETTCompanionId GetId() const { return Id; }
	FString GetDisplayName() const;
	ETTCompanionState GetState() const { return State; }
	FString GetStateName() const;
	bool IsActive() const { return State != ETTCompanionState::Escaped && State != ETTCompanionState::Taken && State != ETTCompanionState::Dormant; }
	bool IsInLocker() const { return HiddenIn.IsValid(); }
	ATTLocker* GetHiddenLocker() const { return HiddenIn.Get(); }
	bool IsVisibleTarget() const { return IsActive() && !IsInLocker(); }
	float GetVisibility() const;
	FTTCompanionMind* GetMind() const { return Mind.Get(); }
	float GetTrust() const;
	float GetLoyalty() const;
	float GetFear() const;
	FString GetTraitSummary() const;

	// Joe interactions
	FString GetPrompt(const ATTPlayerCharacter* Joe) const;
	void Talk(ATTPlayerCharacter* Joe);
	void RequestFollow(ATTPlayerCharacter* Joe);
	/** Called by Joe after IntegrationCore::ShareAmmunition succeeded. */
	void OnReceivedAmmo();
	/** The Hunter got stunned while chasing this companion (Joe saved them). */
	void OnSavedByJoe();

	// Hunter interactions
	void OnChasedByHunter(bool bStarted);
	/** Taken by the Hunter. Returns what they knew about Joe (for the interrogation). */
	bool TakenByHunter(FVector& OutJoeLocation, FString& OutHideSpotId);
	void OnFriendTaken(const ATTCompanion* Friend);

	/** Test hook. */
	void DebugForceGoal(int32 Goal);

private:
	ATTHospital* GetHospital() const;
	ATTHunter* GetHunter() const;
	ATTPlayerCharacter* GetJoe() const;
	double Now() const;
	FString NextEventId(const TCHAR* Kind);

	void BuildVisual();
	void SetState(ETTCompanionState NewState);
	void Say(const FString& Line, float Duration = 4.f);
	bool CanSee(const AActor* Target, const FVector& Point, float Range) const;
	void UpdatePerception(float DeltaSeconds);
	void Decide();
	void ExecuteGoal(int32 Goal, const FString& RouteId);

	void StartHiding();
	void EnterLocker(ATTLocker* Locker);
	void LeaveLocker();
	void StartFlee();
	void StartSearchResource();
	void StartGoToJoe(bool bToShareRoute);
	void StartEscape();
	void StartBetrayal();
	void ShareRouteWithJoe();
	FString NextUnsharedRoute() const;
	void ApplyInfluence(float Trust, float Fear, float Stress, float Loyalty, float SelfPreservation, float Relationship, int32 Ammo, const TCHAR* Kind);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;

	TSharedPtr<FTTCompanionMind> Mind;
	ETTCompanionId Id = ETTCompanionId::Harley;
	ETTDifficulty Difficulty = ETTDifficulty::Normal;
	ETTCompanionState State = ETTCompanionState::Dormant;
	float StateTime = 0.f;
	float DecisionTimer = 3.f;
	float PerceptionTimer = 0.f;
	float LineCooldown = 0.f;
	int32 EventCounter = 0;

	FTTPathFollower Mover;
	TWeakObjectPtr<ATTLocker> HiddenIn;
	TWeakObjectPtr<ATTLocker> TargetLocker;
	TWeakObjectPtr<ATTPickup> TargetPickup;
	TWeakObjectPtr<ATTWindow> TargetWindow;

	bool bSeesHunter = false;
	float HunterSeenAt = -100.f;
	FVector HunterSeenLocation = FVector::ZeroVector;
	bool bSeesJoe = false;
	double JoeSeenAt = -1000.0;
	FVector JoeSeenLocation = FVector::ZeroVector;
	FString JoeSeenHidingIn;
	bool bShareRouteOnArrival = false;
	TSet<FString> SharedRoutes;
	bool bHasCrowbar = false;
	bool bGaveAid = false;
	float FollowRequestTime = -100.f;
	float LastHelpShot = -100.f;

	bool bChased = false;
	bool bJoeWitnessedChase = false;
	bool bSavedDuringChase = false;
	float JoeDistanceAtChase = 0.f;
};
