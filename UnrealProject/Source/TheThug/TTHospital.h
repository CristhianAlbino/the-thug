#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TTHospital.generated.h"

class ACameraActor;
class ATTDoor;
class ATTHatch;
class ATTLocker;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * The whole hospital (M05 blockout): garage, B1, floors 1-4 and the roof, stacked vertically,
 * generated from the ASCII floors in TTHospitalLayout.inl (authored in Tools/MapCore/blockout_floors.py).
 *
 * Cells are FIntPoint on one combined grid: floor N occupies columns [N*Stride, N*Stride+FloorW).
 * That keeps every cell id unique across floors; stairs and service hatches are explicit links.
 */
UCLASS()
class THETHUG_API ATTHospital : public AActor
{
	GENERATED_BODY()

public:
	ATTHospital();

	static constexpr float CellSize = 300.f;
	static constexpr float FloorHeight = 400.f;
	static constexpr float SlabThickness = 20.f;
	static constexpr float WallHeight = FloorHeight - SlabThickness;
	static constexpr float ParapetHeight = 110.f;
	static constexpr int32 FloorW = 24;
	static constexpr int32 FloorH = 15;
	static constexpr int32 Stride = FloorW + 8;
	/** Index of floor 1 (Emergência), which sits at Z = 0. */
	static constexpr int32 GroundFloorIndex = 2;

	void Build();
	virtual void Tick(float DeltaSeconds) override;

	// Coordinates
	FVector CellToWorld(const FIntPoint& Cell, float Z = 0.f) const;
	FIntPoint WorldToCell(const FVector& Location) const;
	int32 FloorOfCell(const FIntPoint& Cell) const { return FMath::Clamp(Cell.X / Stride, 0, FMath::Max(0, Floors.Num() - 1)); }
	int32 FloorById(const FString& Id) const;
	int32 FloorOfLocation(const FVector& Location) const { return FloorOfCell(WorldToCell(Location)); }
	float FloorZ(int32 Floor) const;
	int32 GetFloorCount() const { return Floors.Num(); }
	FString GetFloorName(int32 Floor) const;
	FString GetFloorLabel(int32 Floor) const;

	// Navigation (Hunter, companions)
	bool IsWalkable(const FIntPoint& Cell) const;
	FIntPoint FindNearestWalkable(const FIntPoint& Cell) const;
	bool FindPath(const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPath, bool bForHunter = false) const;
	TArray<FIntPoint> GetWalkableCellsNear(const FIntPoint& Center, int32 Radius) const;
	/** Generators in B1 (MapCore C_SERVICE): garage lights, gate control and the elevator. */
	void SetPowered(bool bInPowered);
	bool IsPowered() const { return bPowered; }
	/** Once Joe has found the tunnel grates, the Hunter can follow him down there. */
	void EnableTunnelLinks();
	void BreakAllCameras();
	/** The old isolation room on 3: the Hunter only goes in once he saw Joe use it. */
	bool IsIsolationCell(const FIntPoint& Cell) const { return IsolationCells.Contains(Cell); }
	void SetHunterKnowsIsolation() { bHunterKnowsIsolation = true; }

	ATTDoor* GetDoorAt(const FIntPoint& Cell) const;
	const TMap<FIntPoint, TObjectPtr<ATTDoor>>& GetDoors() const { return Doors; }
	static FString DoorId(const FIntPoint& Cell) { return FString::Printf(TEXT("door_%d_%d"), Cell.X, Cell.Y); }
	static FString LockerId(const FIntPoint& Cell) { return FString::Printf(TEXT("locker_%d_%d"), Cell.X, Cell.Y); }
	bool HasLockedDoor() const;
	/** True when every walkable cell stays reachable for Joe with Blocked closed off. */
	bool IsConnectedWithout(const FIntPoint& Blocked) const;
	/** The Hunter manipulating the environment (GDD §14). Returns how many lamps changed. */
	int32 SetLampsNear(const FVector& Center, float Radius, bool bOn);
	bool IsGlassAt(const FVector& Location) const { return GlassCells.Contains(WorldToCell(Location)); }

	const TArray<TObjectPtr<ATTLocker>>& GetLockers() const { return Lockers; }
	const TArray<FIntPoint>& GetPatrolPoints() const { return PatrolPoints; }
	const TArray<TObjectPtr<ACameraActor>>& GetSecurityCameras() const { return SecurityCameras; }
	const TArray<FString>& GetSecurityCameraNames() const { return SecurityCameraNames; }
	bool IsSecurityCameraBroken(int32 Index) const { return BrokenCameras.Contains(Index); }

	FIntPoint GetPlayerStartCell() const { return PlayerStart; }
	FIntPoint GetHunterStartCell() const { return HunterStart; }
	float GetPlayerStartYaw() const { return PlayerStartYaw; }
	bool GetCompanionStartCell(int32 Index, FIntPoint& OutCell) const
	{
		const FIntPoint* Found = CompanionStarts.Find(Index);
		if (Found)
		{
			OutCell = *Found;
		}
		return Found != nullptr;
	}

	/** 0 = darkness, 1 = standing under a working lamp. */
	float GetLightLevelAt(const FVector& Location, const AActor* IgnoreActor = nullptr) const;

private:
	struct FFloor
	{
		FString Id;
		FString Name;
		bool bOutdoor = false;
		float Z = 0.f;
		bool bStairs = true;
		TArray<FString> Rows;
	};

	struct FLamp
	{
		TObjectPtr<UPointLightComponent> Light = nullptr;
		TObjectPtr<UStaticMeshComponent> Fixture = nullptr;
		TObjectPtr<UMaterialInstanceDynamic> Glow = nullptr;
		float BaseIntensity = 0.f;
		int32 Floor = 0;
		bool bFlicker = false;
		bool bOn = true;
		bool bKilled = false;
		bool bActive = true;
		bool bNeedsPower = false;
		float Timer = 0.f;
	};

	enum class EStairCell : uint8 { None, Walk, Ramp, Void, Wall };

	TCHAR GetChar(const FIntPoint& Cell) const;
	TCHAR GetLocalChar(int32 Floor, int32 X, int32 Y) const;
	EStairCell StairCellAt(int32 Floor, int32 X, int32 Y) const;
	void BuildStairs(int32 Floor);
	void SpawnAtmosphere();
	void SpawnCity();
	void AddLamp(const FIntPoint& Cell, bool bFlicker);
	float FacingYawAwayFromWall(const FIntPoint& Cell) const;
	void AddLink(const FIntPoint& A, const FIntPoint& B);
	void UpdateLampCulling();

	TArray<FFloor> Floors;
	int32 GridWidth = 0;
	TArray<bool> Walkable;
	TMultiMap<FIntPoint, FIntPoint> Links;
	TSet<FIntPoint> IsolationCells;
	bool bHunterKnowsIsolation = false;
	bool bPowered = false;
	bool bTunnelLinks = false;
	TArray<FIntPoint> TunnelLadders;

	FIntPoint PlayerStart = FIntPoint::ZeroValue;
	FIntPoint HunterStart = FIntPoint::ZeroValue;
	float PlayerStartYaw = 0.f;
	TArray<FIntPoint> PatrolPoints;
	TArray<FLamp> Lamps;
	TSet<FIntPoint> GlassCells;
	TMap<int32, FIntPoint> CompanionStarts;
	TArray<FString> SecurityCameraNames;
	TSet<int32> BrokenCameras;
	int32 LastCullFloor = -1;
	float CullTimer = 0.f;

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<ATTDoor>> Doors;

	UPROPERTY()
	TArray<TObjectPtr<ATTLocker>> Lockers;

	UPROPERTY()
	TArray<TObjectPtr<ACameraActor>> SecurityCameras;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Slabs;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Walls;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WallTrim;

	bool bBuilt = false;
};
