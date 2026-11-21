#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TTTypes.h"
#include "TTPlayerCharacter.generated.h"

class ATTCompanion;
class ATTHatch;
class ATTInteractable;
class ATTLocker;
class ATTWindow;
class UCameraComponent;
class USpotLightComponent;
struct FTTJoeBody;

UENUM()
enum class ETTJoeAction : uint8
{
	None,
	Bandage,
	Medkit,
	Reload,
	Pry,
	Crawl
};

/**
 * Joe Chill: first-person, vulnerable, noisy when careless.
 * Body (injury, bleeding, stamina, collapse, death) and inventory come from
 * thug::survival::SurvivalCore and thug::survival::Inventory in /Tools.
 */
UCLASS()
class THETHUG_API ATTPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATTPlayerCharacter();
	virtual ~ATTPlayerCharacter() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// Input (driven by ATTPlayerController)
	void SetMoveInput(const FVector2D& Input) { MoveInput = Input; }
	void SetSprintHeld(bool bHeld) { bSprintHeld = bHeld; }
	void SetCrouchHeld(bool bHeld) { bCrouchHeld = bHeld; }
	void ToggleCrouch() { bCrouchToggled = !bCrouchToggled; }
	void ToggleFlashlight();
	void Interact();
	void ThrowBottle();
	void ThrowSmoke();
	void ThrowGrenade();
	void SetAiming(bool bInAiming) { bAiming = bInAiming; }
	void Fire();
	void Reload();
	void UseMedical();
	void SetInventoryOpen(bool bOpen) { bInventoryOpen = bOpen; }
	void DropItem(ETTItem Item);
	void GiveAmmoToFocus();
	void RequestFollowFromFocus();
	void StartPrying(ATTWindow* Window);
	void StartCrawl(ATTHatch* Target);
	void StartCameraView();
	void StopCameraView();
	void CycleCamera(int32 Step);
	bool IsViewingCameras() const { return bViewingCameras; }
	int32 GetCameraIndex() const { return CameraIndex; }
	int32 GetFloor() const { return CurrentFloor; }
	/** A companion stops Joe's bleeding (GDD §49 "salvar Joe"). */
	void ReceiveAid(const FString& From);

	// Hiding
	void EnterLocker(ATTLocker* Locker);
	void ExitLocker();
	ATTLocker* GetHiddenLocker() const { return HiddenIn; }
	bool IsHidden() const { return HiddenIn != nullptr; }

	// Inventory
	void ApplyLoadout(ETTDifficulty Difficulty);
	bool CanCarry(ETTItem Item) const;
	bool TryAddItem(ETTItem Item, int32 Count);
	int32 GetItemCount(ETTItem Item) const;
	int32 GetUsedSlots() const;
	int32 GetCapacity() const;
	int32 GetBottles() const { return GetItemCount(ETTItem::Bottle); }
	int32 GetRounds() const { return Rounds; }
	void GiveKey() { bHasKey = true; }
	bool HasKey() const { return bHasKey; }
	bool IsInventoryOpen() const { return bInventoryOpen; }

	// Body
	/** 1 light, 2 moderate, 3 severe (severe bleeds). Damage only ever makes it worse. */
	void TakeInjury(int32 Level, const FString& Reason);
	/** The Hunter hits Joe instead of taking him (difficulty dependent): one level worse, knocked back. */
	void HunterStrike(const FVector& From);
	int32 GetInjury() const;
	bool IsBleeding() const;
	float GetBloodLoss() const;
	bool IsCollapsed() const;
	bool IsDead() const;
	float GetDamageFlash() const { return DamageFlash; }

	// State for HUD / Hunter
	ATTInteractable* GetFocus() const { return Focus; }
	ATTCompanion* GetFocusCompanion() const { return FocusCompanion.Get(); }
	float GetStamina() const;
	bool IsSprinting() const { return bSprinting; }
	bool IsAiming() const { return bAiming; }
	bool IsFlashlightOn() const { return bFlashlightOn; }
	float GetLightLevel() const { return LightLevel; }
	ETTJoeAction GetAction() const { return Action; }
	float GetActionProgress() const { return ActionDuration > 0.f ? ActionTime / ActionDuration : 0.f; }
	/** 0..1, how easy Joe is to spot right now (light, posture, speed). */
	float GetVisibility() const;
	FVector GetEyeLocation() const;

	void SetFrozen(bool bInFrozen);
	bool IsFrozen() const { return bFrozen; }
	void LookAtPoint(const FVector& Target);

	void TeleportForTest(const FVector& Location, float Yaw);

private:
	bool CanAct() const;
	bool CanMove() const;
	void UpdateBody(float DeltaSeconds);
	void UpdateMovement(float DeltaSeconds);
	void UpdateAction(float DeltaSeconds);
	void UpdateFocus();
	void UpdateFootsteps(float DeltaSeconds);
	void UpdateHeartbeat(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds);
	void ReportNoise(ETTNoise Kind);
	void Message(const FString& Text, float Duration = 4.f) const;
	bool Throw(ETTItem Item, int32 ThrowType, float Speed);
	void StartAction(ETTJoeAction NewAction, float Duration);
	void CancelAction();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY()
	TObjectPtr<ATTInteractable> Focus;

	UPROPERTY()
	TObjectPtr<ATTLocker> HiddenIn;

	TWeakObjectPtr<ATTCompanion> FocusCompanion;
	TWeakObjectPtr<ATTWindow> PryTarget;
	TWeakObjectPtr<ATTHatch> CrawlTarget;
	bool bViewingCameras = false;
	int32 CameraIndex = 0;
	int32 CurrentFloor = -1;
	int32 ShareCounter = 0;

	TSharedPtr<FTTJoeBody> Body;

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bSprintHeld = false;
	bool bSprinting = false;
	bool bCrouchToggled = false;
	bool bCrouchHeld = false;
	bool bFlashlightOn = true;
	bool bFlashlightBeforeHiding = true;
	bool bFrozen = false;
	bool bHasKey = false;
	bool bAiming = false;
	bool bInventoryOpen = false;
	bool bDeathReported = false;

	int32 Rounds = 10;
	float FireCooldown = 0.f;

	ETTJoeAction Action = ETTJoeAction::None;
	float ActionTime = 0.f;
	float ActionDuration = 0.f;

	float BreathTimer = 0.f;
	float StepDistance = 0.f;
	float BobPhase = 0.f;
	float LightLevel = 0.f;
	float LightTimer = 0.f;
	float HeartTimer = 0.f;
	float FlashlightFlickerTimer = 0.f;
	float CameraBaseZ = 0.f;
	float CrouchCameraOffset = 0.f;
	float CollapseCameraOffset = 0.f;
	float DamageFlash = 0.f;
	float ShakeTime = 0.f;
	float GlassInjuryCooldown = 0.f;
	float Fov = 88.f;
	float FlashlightPower = 12.f;
};
