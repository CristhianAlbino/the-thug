#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTHospital.generated.h"

class ATTDoor;
class ATTLocker;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * M01 slice of the 4th floor, generated from an ASCII layout at runtime.
 * Also owns the grid used by the Hunter for path finding (no navmesh needed).
 */
UCLASS()
class THETHUG_API ATTHospital : public AActor
{
	GENERATED_BODY()

public:
	ATTHospital();

	static constexpr float CellSize = 300.f;
	static constexpr float WallHeight = 330.f;

	void Build();
	virtual void Tick(float DeltaSeconds) override;

	FVector CellToWorld(const FIntPoint& Cell, float Z = 0.f) const;
	FIntPoint WorldToCell(const FVector& Location) const;
	bool IsWalkable(const FIntPoint& Cell) const;
	FIntPoint FindNearestWalkable(const FIntPoint& Cell) const;
	bool FindPath(const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPath) const;
	TArray<FIntPoint> GetWalkableCellsNear(const FIntPoint& Center, int32 Radius) const;

	ATTDoor* GetDoorAt(const FIntPoint& Cell) const;
	const TArray<TObjectPtr<ATTLocker>>& GetLockers() const { return Lockers; }
	const TArray<FIntPoint>& GetPatrolPoints() const { return PatrolPoints; }

	FIntPoint GetPlayerStartCell() const { return PlayerStart; }
	FIntPoint GetHunterStartCell() const { return HunterStart; }
	float GetPlayerStartYaw() const { return PlayerStartYaw; }

	/** 0 = darkness, 1 = standing under a working lamp. */
	float GetLightLevelAt(const FVector& Location, const AActor* IgnoreActor = nullptr) const;

private:
	struct FLamp
	{
		TObjectPtr<UPointLightComponent> Light = nullptr;
		TObjectPtr<UStaticMeshComponent> Fixture = nullptr;
		float BaseIntensity = 0.f;
		bool bFlicker = false;
		bool bOn = true;
		float Timer = 0.f;
	};

	TCHAR GetChar(int32 X, int32 Y) const;
	void SpawnAtmosphere();
	void AddLamp(const FIntPoint& Cell, bool bFlicker, bool bDead);
	float FacingYawAwayFromWall(const FIntPoint& Cell) const;

	TArray<FString> Rows;
	int32 Width = 0;
	int32 Height = 0;
	TArray<bool> Walkable;

	FIntPoint PlayerStart = FIntPoint::ZeroValue;
	FIntPoint HunterStart = FIntPoint::ZeroValue;
	float PlayerStartYaw = 0.f;
	TArray<FIntPoint> PatrolPoints;
	TArray<FLamp> Lamps;

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<ATTDoor>> Doors;

	UPROPERTY()
	TArray<TObjectPtr<ATTLocker>> Lockers;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WallTrim;

	bool bBuilt = false;
};
