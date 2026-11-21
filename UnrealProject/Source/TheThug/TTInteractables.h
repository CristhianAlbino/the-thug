#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTTypes.h"
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
	Item
};

UCLASS()
class THETHUG_API ATTPickup : public ATTInteractable
{
	GENERATED_BODY()

public:
	void Setup(ETTPickupType InType, ETTItem InItem = ETTItem::Bottle, bool bOnTable = false);

	virtual FString GetPrompt(const ATTPlayerCharacter* Player) const override;
	virtual void Interact(ATTPlayerCharacter* Player) override;

	ETTPickupType GetType() const { return Type; }
	ETTItem GetItem() const { return Item; }

private:
	ETTPickupType Type = ETTPickupType::Item;
	ETTItem Item = ETTItem::Bottle;
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

UENUM()
enum class ETTThrowableType : uint8
{
	Bottle,
	Smoke,
	Grenade
};

/**
 * Thrown objects. Bottle: shatters on impact (distraction).
 * Smoke: rolls, then releases a cloud that blocks the Hunter's sight.
 * Grenade: bounces, explodes after a fuse (stuns him, breaks lights, blows doors, hurts Joe if close).
 */
UCLASS()
class THETHUG_API ATTThrowable : public AActor
{
	GENERATED_BODY()

public:
	ATTThrowable();

	void Launch(const FVector& Velocity, AActor* Thrower, ETTThrowableType InType);
	virtual void Tick(float DeltaSeconds) override;

protected:
	UFUNCTION()
	void OnStop(const FHitResult& ImpactResult);

	void Explode();
	void ReleaseSmoke();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

	ETTThrowableType Type = ETTThrowableType::Bottle;
	bool bDone = false;
	bool bStopped = false;
	float Age = 0.f;
	float StoppedTime = 0.f;
};

/** Smoke cloud from a smoke grenade (local fog volume). The Hunter cannot see through it. */
UCLASS()
class THETHUG_API ATTSmokeCloud : public AActor
{
	GENERATED_BODY()

public:
	ATTSmokeCloud();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	float GetRadius() const { return Radius; }
	bool BlocksSegment(const FVector& A, const FVector& B) const;

private:
	struct FPuff
	{
		TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
		TObjectPtr<UMaterialInstanceDynamic> Material = nullptr;
		FVector Offset = FVector::ZeroVector;
		FVector Drift = FVector::ZeroVector;
		float Scale = 1.f;
		float Spin = 0.f;
	};

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	TArray<FPuff> Puffs;
	float Age = 0.f;
	float Radius = 0.f;
};

/** Short bright flash (muzzle flash, explosion). */
UCLASS()
class THETHUG_API ATTFlash : public AActor
{
	GENERATED_BODY()

public:
	ATTFlash();
	void Setup(float Lumens, float AttenuationRadius, float Duration, const FLinearColor& Color);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UPointLightComponent> Light;

	float Peak = 0.f;
	float Life = 0.1f;
	float Age = 0.f;
};
