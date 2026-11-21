#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTInteractables.generated.h"

class ATTPlayerCharacter;
class UPointLightComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/** Base for everything the player can use with [E]. */
UCLASS(Abstract)
class THETHUG_API ATTInteractable : public AActor
{
	GENERATED_BODY()

public:
	ATTInteractable();

	virtual FString GetPrompt(const ATTPlayerCharacter* Player) const { return FString(); }
	virtual void Interact(ATTPlayerCharacter* Player) {}

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
};

/** Hinged door. Crouching while opening it is slow and quiet; a normal open creaks and makes noise. */
UCLASS()
class THETHUG_API ATTDoor : public ATTInteractable
{
	GENERATED_BODY()

public:
	ATTDoor();

	void Setup(float CellSize, float WallHeight, const FString& InId);
	virtual void Tick(float DeltaSeconds) override;

	virtual FString GetPrompt(const ATTPlayerCharacter* Player) const override;
	virtual void Interact(ATTPlayerCharacter* Player) override;

	const FString& GetId() const { return Id; }
	bool IsOpen() const { return bOpen; }
	/** Locked doors stop Joe only; the Hunter wedged it and can still pass. */
	bool IsLocked() const { return bLocked; }
	void SetLocked(bool bInLocked) { bLocked = bInLocked; }
	bool IsMoving() const { return !FMath::IsNearlyEqual(Angle, TargetAngle, 1.f); }
	void SetOpen(bool bInOpen, bool bQuiet, const AActor* Opener);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Hinge;

	FString Id;
	bool bOpen = false;
	bool bLocked = false;
	float Angle = 0.f;
	float TargetAngle = 0.f;
	float Speed = 200.f;
};

/** Locker the player can hide in. The Hunter remembers how often it is used. */
UCLASS()
class THETHUG_API ATTLocker : public ATTInteractable
{
	GENERATED_BODY()

public:
	ATTLocker();

	void Setup(const FString& InId);
	virtual void Tick(float DeltaSeconds) override;

	virtual FString GetPrompt(const ATTPlayerCharacter* Player) const override;
	virtual void Interact(ATTPlayerCharacter* Player) override;

	const FString& GetId() const { return Id; }
	FVector GetInsideLocation() const;
	FVector GetFrontLocation() const;
	float GetFacingYaw() const { return GetActorRotation().Yaw; }

	int32 GetUses() const { return Uses; }

	bool IsOccupied() const { return bOccupied; }
	void SetOccupied(bool bInOccupied);
	void FlapDoor();

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> DoorHinge;

	FString Id;
	bool bOccupied = false;
	int32 Uses = 0;
	float DoorAngle = 0.f;
	float DoorTimer = 0.f;
};

UENUM()
enum class ETTPickupType : uint8
{
	Key,
	Bottle
};

UCLASS()
class THETHUG_API ATTPickup : public ATTInteractable
{
	GENERATED_BODY()

public:
	void Setup(ETTPickupType InType);

	virtual FString GetPrompt(const ATTPlayerCharacter* Player) const override;
	virtual void Interact(ATTPlayerCharacter* Player) override;

	ETTPickupType GetType() const { return Type; }

private:
	ETTPickupType Type = ETTPickupType::Bottle;
};

/** Locked service gate. Needs the gate keycard. */
UCLASS()
class THETHUG_API ATTExitGate : public ATTInteractable
{
	GENERATED_BODY()

public:
	void Setup(float CellSize, float WallHeight);

	virtual FString GetPrompt(const ATTPlayerCharacter* Player) const override;
	virtual void Interact(ATTPlayerCharacter* Player) override;

	void SetUnlocked();

private:
	UPROPERTY()
	TObjectPtr<UPointLightComponent> StatusLight;
};

/** Thrown bottle: shatters on impact and creates a loud distraction. */
UCLASS()
class THETHUG_API ATTThrowable : public AActor
{
	GENERATED_BODY()

public:
	ATTThrowable();

	void Launch(const FVector& Velocity, AActor* Thrower);

protected:
	UFUNCTION()
	void OnStop(const FHitResult& ImpactResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

	bool bBroken = false;
};
