#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TTPlayerCharacter.generated.h"

class ATTInteractable;
class ATTLocker;
class UCameraComponent;
class USpotLightComponent;

/** Joe Chill: first-person, vulnerable, noisy when careless. */
UCLASS()
class THETHUG_API ATTPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATTPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// Input (driven by ATTPlayerController)
	void SetMoveInput(const FVector2D& Input) { MoveInput = Input; }
	void SetSprintHeld(bool bHeld) { bSprintHeld = bHeld; }
	void SetCrouchHeld(bool bHeld);
	void ToggleCrouch();
	void ToggleFlashlight();
	void Interact();
	void ThrowBottle();

	// Hiding
	void EnterLocker(ATTLocker* Locker);
	void ExitLocker();
	ATTLocker* GetHiddenLocker() const { return HiddenIn; }
	bool IsHidden() const { return HiddenIn != nullptr; }

	// Inventory
	void GiveKey() { bHasKey = true; }
	bool HasKey() const { return bHasKey; }
	void AddBottles(int32 Count) { Bottles += Count; }
	int32 GetBottles() const { return Bottles; }

	// State for HUD / Hunter
	ATTInteractable* GetFocus() const { return Focus; }
	float GetStamina() const { return Stamina; }
	bool IsSprinting() const { return bSprinting; }
	bool IsFlashlightOn() const { return bFlashlightOn; }
	float GetLightLevel() const { return LightLevel; }
	/** 0..1, how easy Joe is to spot right now (light, posture, speed). */
	float GetVisibility() const;
	FVector GetEyeLocation() const;

	void SetFrozen(bool bInFrozen);
	bool IsFrozen() const { return bFrozen; }
	void LookAtPoint(const FVector& Target);

	void TeleportForTest(const FVector& Location, float Yaw);

private:
	void UpdateMovement(float DeltaSeconds);
	void UpdateFocus();
	void UpdateFootsteps(float DeltaSeconds);
	void UpdateHeartbeat(float DeltaSeconds);
	void ReportNoise(float Loudness);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY()
	TObjectPtr<ATTInteractable> Focus;

	UPROPERTY()
	TObjectPtr<ATTLocker> HiddenIn;

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bSprintHeld = false;
	bool bSprinting = false;
	bool bCrouchToggled = false;
	bool bCrouchHeld = false;
	bool bFlashlightOn = true;
	bool bFrozen = false;
	bool bHasKey = false;
	int32 Bottles = 1;

	float Stamina = 1.f;
	bool bExhausted = false;
	float StaminaRegenDelay = 0.f;
	float BreathTimer = 0.f;

	float StepDistance = 0.f;
	float BobPhase = 0.f;
	float LightLevel = 0.f;
	float LightTimer = 0.f;
	float HeartTimer = 0.f;
	float FlashlightFlickerTimer = 0.f;
	float CameraBaseZ = 0.f;
	float CrouchCameraOffset = 0.f;
	bool bFlashlightBeforeHiding = true;
};
