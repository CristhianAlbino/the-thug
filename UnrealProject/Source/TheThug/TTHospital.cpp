#include "TTHospital.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TTAudio.h"
#include "TTInteractables.h"
#include "TTShapes.h"

namespace
{
	struct FFloorSource
	{
		const TCHAR* Id;
		const TCHAR* Name;
		bool bOutdoor;
		float Z;
		bool bStairs;
		const TCHAR* Rows[ATTHospital::FloorH];
	};

	const FFloorSource FloorSources[] = {
#include "TTHospitalLayout.inl"
	};

	struct FZoneSource
	{
		const TCHAR* Floor;
		const TCHAR* Node;
		int32 X0, Y0, X1, Y1;
	};

	struct FDocSource
	{
		const TCHAR* Floor;
		int32 X, Y;
		const TCHAR* Id;
	};

#include "TTHospitalZones.inl"

	/** Stairwells: (lane A column, top row). Two switchback lanes, four rows. */
	const FIntPoint StairDefs[] = { FIntPoint(1, 1), FIntPoint(21, 1) };

	const FLinearColor WallColor(0.34f, 0.37f, 0.33f);
	const FLinearColor TrimColor(0.12f, 0.17f, 0.15f);
	const FLinearColor FloorColor(0.16f, 0.16f, 0.15f);
	const FLinearColor ConcreteColor(0.2f, 0.2f, 0.2f);
	const FLinearColor BedColor(0.30f, 0.31f, 0.30f);
	const FLinearColor SheetColor(0.42f, 0.42f, 0.38f);
	const FLinearColor DeskColor(0.2f, 0.15f, 0.1f);

	const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

	bool IsWalkableChar(TCHAR C)
	{
		switch (C)
		{
		case TEXT('.'): case TEXT('*'): case TEXT('~'): case TEXT('D'): case TEXT('L'):
		case TEXT('P'): case TEXT('H'): case TEXT('b'): case TEXT('a'): case TEXT('n'):
		case TEXT('s'): case TEXT('x'): case TEXT('g'): case TEXT('c'):
		case TEXT('Y'): case TEXT('U'): case TEXT('J'):
		case TEXT('V'): case TEXT('v'): case TEXT('@'): case TEXT('i'): case TEXT('I'):
		case TEXT('%'): case TEXT('!'): case TEXT('T'):
			return true;
		default:
			return C >= TEXT('1') && C <= TEXT('9');
		}
	}

	bool IsWallChar(TCHAR C)
	{
		return C == TEXT('#') || C == TEXT('w') || C == TEXT('E') || C == TEXT('W') || C == TEXT('X');
	}
}

ATTHospital::ATTHospital()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// ---------------------------------------------------------------------------
// Coordinates

float ATTHospital::FloorZ(int32 Floor) const
{
	return Floors.IsValidIndex(Floor) ? Floors[Floor].Z : (Floor - GroundFloorIndex) * FloorHeight;
}

int32 ATTHospital::FloorById(const FString& Id) const
{
	for (int32 i = 0; i < Floors.Num(); ++i)
	{
		if (Floors[i].Id == Id)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FVector ATTHospital::CellToWorld(const FIntPoint& Cell, float Z) const
{
	const int32 Floor = FloorOfCell(Cell);
	const int32 LocalX = Cell.X - Floor * Stride;
	return GetActorLocation() + FVector(LocalX * CellSize, Cell.Y * CellSize, FloorZ(Floor) + Z);
}

FIntPoint ATTHospital::WorldToCell(const FVector& Location) const
{
	const FVector Local = Location - GetActorLocation();
	// +100: an actor standing on a floor (or an object lying on it) belongs to that floor;
	// on a staircase the cell switches to the upper floor in the last quarter of the flight.
	int32 Floor = 0;
	float BestZ = -TNumericLimits<float>::Max();
	float LowestZ = TNumericLimits<float>::Max();
	int32 Lowest = 0;
	for (int32 i = 0; i < Floors.Num(); ++i)
	{
		const float FZ = Floors[i].Z;
		if (FZ <= Local.Z + 100.f && FZ > BestZ)
		{
			BestZ = FZ;
			Floor = i;
		}
		if (FZ < LowestZ)
		{
			LowestZ = FZ;
			Lowest = i;
		}
	}
	if (BestZ == -TNumericLimits<float>::Max())
	{
		Floor = Lowest;
	}
	const int32 X = FMath::Clamp(FMath::RoundToInt(Local.X / CellSize), 0, FloorW - 1);
	const int32 Y = FMath::Clamp(FMath::RoundToInt(Local.Y / CellSize), 0, FloorH - 1);
	return FIntPoint(Floor * Stride + X, Y);
}

FString ATTHospital::GetFloorName(int32 Floor) const
{
	return Floors.IsValidIndex(Floor) ? Floors[Floor].Name : FString();
}

FString ATTHospital::GetFloorLabel(int32 Floor) const
{
	if (!Floors.IsValidIndex(Floor))
	{
		return FString();
	}
	const FString& Id = Floors[Floor].Id;
	if (Id.StartsWith(TEXT("F")))
	{
		return FString::Printf(TEXT("%sº andar — %s"), *Id.RightChop(1), *Floors[Floor].Name);
	}
	return Floors[Floor].Name;
}

TCHAR ATTHospital::GetLocalChar(int32 Floor, int32 X, int32 Y) const
{
	if (!Floors.IsValidIndex(Floor) || Y < 0 || Y >= FloorH || X < 0 || X >= FloorW)
	{
		return TEXT('#');
	}
	const EStairCell Stair = StairCellAt(Floor, X, Y);
	if (Stair == EStairCell::Wall)
	{
		return TEXT('#');
	}
	if (Stair != EStairCell::None)
	{
		return TEXT('S');
	}
	return Floors[Floor].Rows[Y][X];
}

TCHAR ATTHospital::GetChar(const FIntPoint& Cell) const
{
	const int32 Floor = Cell.X / Stride;
	return GetLocalChar(Floor, Cell.X - Floor * Stride, Cell.Y);
}

ATTHospital::EStairCell ATTHospital::StairCellAt(int32 Floor, int32 X, int32 Y) const
{
	if (!Floors.IsValidIndex(Floor) || !Floors[Floor].bStairs)
	{
		return EStairCell::None;
	}
	for (const FIntPoint& Def : StairDefs)
	{
		const int32 A = Def.X;
		const int32 B = Def.X + 1;
		const int32 R0 = Def.Y;
		if ((X != A && X != B) || Y < R0 || Y > R0 + 3)
		{
			continue;
		}
		const bool bEven = Floor % 2 == 0;
		const bool bTop = !(Floors.IsValidIndex(Floor + 1) && Floors[Floor + 1].bStairs);
		const bool bBottom = Floor == 0;
		const int32 Row = Y - R0;
		if (bEven)
		{
			if (Row == 0) return EStairCell::Wall;
			if (Row == 3) return EStairCell::Walk;
			if (X == A) return bTop ? EStairCell::Walk : EStairCell::Ramp;
			return bBottom ? EStairCell::Wall : EStairCell::Void;
		}
		if (Row == 0) return EStairCell::Walk;
		if (Row == 3) return EStairCell::Wall;
		if (X == B) return bTop ? EStairCell::Walk : EStairCell::Ramp;
		return EStairCell::Void;
	}
	return EStairCell::None;
}

bool ATTHospital::IsWalkable(const FIntPoint& Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= GridWidth || Cell.Y >= FloorH)
	{
		return false;
	}
	return Walkable[Cell.Y * GridWidth + Cell.X];
}

float ATTHospital::FacingYawAwayFromWall(const FIntPoint& Cell) const
{
	for (const FIntPoint& D : Dirs)
	{
		const TCHAR Neighbor = GetChar(Cell + D);
		const TCHAR Opposite = GetChar(Cell - D);
		if (IsWallChar(Neighbor) && !IsWallChar(Opposite))
		{
			return FVector(static_cast<float>(-D.X), static_cast<float>(-D.Y), 0.f).Rotation().Yaw;
		}
	}
	return 0.f;
}

void ATTHospital::AddLink(const FIntPoint& A, const FIntPoint& B)
{
	Links.Add(A, B);
	Links.Add(B, A);
}

// ---------------------------------------------------------------------------
// Build

void ATTHospital::Build()
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;

	for (const FFloorSource& Source : FloorSources)
	{
		FFloor Floor;
		Floor.Id = Source.Id;
		Floor.Name = Source.Name;
		Floor.bOutdoor = Source.bOutdoor;
		Floor.Z = Source.Z;
		Floor.bStairs = Source.bStairs;
		for (int32 Y = 0; Y < FloorH; ++Y)
		{
			Floor.Rows.Add(Source.Rows[Y]);
		}
		Floors.Add(Floor);
	}
	GridWidth = Floors.Num() * Stride;
	Walkable.Init(false, GridWidth * FloorH);

	UWorld* World = GetWorld();
	UStaticMesh* Cube = TTShapes::Cube();

	auto MakeIsm = [this, Cube](const TCHAR* Name, const FLinearColor& Color, float Roughness, bool bCollide)
	{
		UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this, Name);
		Ism->SetStaticMesh(Cube);
		Ism->SetupAttachment(RootComponent);
		Ism->SetMaterial(0, TTShapes::MakeColor(Ism, Color, Roughness));
		if (!bCollide)
		{
			Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Ism->RegisterComponent();
		return Ism;
	};
	Slabs = MakeIsm(TEXT("Slabs"), FloorColor, 0.85f, true);
	Walls = MakeIsm(TEXT("Walls"), WallColor, 0.9f, true);
	WallTrim = MakeIsm(TEXT("WallTrim"), TrimColor, 0.5f, false);
	UInstancedStaticMeshComponent* Concrete = MakeIsm(TEXT("Concrete"), ConcreteColor, 0.95f, true);

	FRandomStream Rng(4242);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	int32 CameraIndex = 0;

	for (int32 F = 0; F < Floors.Num(); ++F)
	{
		const bool bOutdoor = Floors[F].bOutdoor;
		const bool bUnderground = Floors[F].Z < 0.f;
		const float Z0 = FloorZ(F);

		for (int32 Y = 0; Y < FloorH; ++Y)
		{
			for (int32 X = 0; X < FloorW; ++X)
			{
				const FIntPoint Cell(F * Stride + X, Y);
				const FVector Local(X * CellSize, Y * CellSize, Z0);
				const FVector WorldLoc = CellToWorld(Cell);
				const EStairCell Stair = StairCellAt(F, X, Y);
				const TCHAR C = GetLocalChar(F, X, Y);

				// Floor slab (holes where a staircase climbs through).
				if (Stair != EStairCell::Void)
				{
					(bUnderground || bOutdoor ? Concrete : Slabs.Get())->AddInstance(FTransform(FRotator::ZeroRotator, Local - FVector(0.f, 0.f, SlabThickness * 0.5f),
						FVector(CellSize, CellSize, SlabThickness) / 100.f));
				}
				Walkable[Y * GridWidth + Cell.X] = Stair == EStairCell::Walk || Stair == EStairCell::Ramp || (Stair == EStairCell::None && IsWalkableChar(C));
				if (Stair != EStairCell::None && Stair != EStairCell::Wall)
				{
					continue;
				}

				switch (C)
				{
				case TEXT('#'):
				{
					const bool bBorder = X == 0 || Y == 0 || X == FloorW - 1 || Y == FloorH - 1;
					const float Height = (bOutdoor && bBorder) ? ParapetHeight : WallHeight;
					(bUnderground ? Concrete : Walls.Get())->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, Height * 0.5f),
						FVector(CellSize, CellSize, Height) / 100.f));
					if (!bOutdoor && !bUnderground)
					{
						WallTrim->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, 55.f), FVector(CellSize + 4.f, CellSize + 4.f, 110.f) / 100.f));
					}
					break;
				}

				case TEXT('w'):
				{
					// Sealed window: the city out there, unreachable.
					Walls->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, 50.f), FVector(CellSize, CellSize, 100.f) / 100.f));
					Walls->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, 360.f), FVector(CellSize, CellSize, 40.f) / 100.f));
					if (UStaticMeshComponent* Glass = TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 220.f), FVector(CellSize - 20.f, CellSize - 20.f, 240.f), FLinearColor::Black))
					{
						// Clear glass: it blocks, but the city and the sky show through.
						Glass->SetHiddenInGame(true);
						Glass->SetCastShadow(false);
					}
					// Window frame and mullions so the opening still reads as glass.
					const FLinearColor Frame(0.1f, 0.11f, 0.1f);
					const bool bWallAlongY = !IsWalkableChar(GetChar(Cell + FIntPoint(0, 1))) && !IsWalkableChar(GetChar(Cell + FIntPoint(0, -1)));
					const FVector Mullion = bWallAlongY ? FVector(16.f, 8.f, 240.f) : FVector(8.f, 16.f, 240.f);
					const FVector Transom = bWallAlongY ? FVector(16.f, CellSize - 20.f, 6.f) : FVector(CellSize - 20.f, 16.f, 6.f);
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 220.f), Mullion, Frame, FRotator::ZeroRotator, false);
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 270.f), Transom, Frame, FRotator::ZeroRotator, false);
					break;
				}

				case TEXT('*'):
				case TEXT('~'):
					if (!bOutdoor)
					{
						AddLamp(Cell, C == TEXT('~'));
						if (Floors[F].Id == TEXT("G"))
						{
							// Garage circuit is dead until the generators run (MapCore C_SERVICE).
							FLamp& Lamp = Lamps.Last();
							Lamp.bNeedsPower = true;
							Lamp.bOn = false;
							Lamp.Light->SetIntensity(0.f);
							if (Lamp.Glow)
							{
								Lamp.Glow->SetScalarParameterValue(TEXT("Intensity"), 0.05f);
							}
						}
					}
					break;

				case TEXT('D'):
				case TEXT('I'):
				{
					const bool bWallsLeftRight = IsWallChar(GetChar(Cell + FIntPoint(-1, 0))) && IsWallChar(GetChar(Cell + FIntPoint(1, 0)));
					const FRotator Rot(0.f, bWallsLeftRight ? 0.f : 90.f, 0.f);
					if (ATTDoor* Door = World->SpawnActor<ATTDoor>(ATTDoor::StaticClass(), WorldLoc, Rot, Params))
					{
						Door->Setup(CellSize, WallHeight, DoorId(Cell));
						if (C == TEXT('I'))
						{
							Door->SetHeavy(true);
							IsolationCells.Add(Cell);
						}
						Doors.Add(Cell, Door);
					}
					break;
				}

				case TEXT('i'):
					IsolationCells.Add(Cell);
					break;

				case TEXT('L'):
				{
					const float Yaw = FacingYawAwayFromWall(Cell);
					const FVector Facing = FRotator(0.f, Yaw, 0.f).Vector();
					const FVector Loc = WorldLoc - Facing * (CellSize * 0.5f - 34.f);
					if (ATTLocker* Locker = World->SpawnActor<ATTLocker>(ATTLocker::StaticClass(), Loc, FRotator(0.f, Yaw, 0.f), Params))
					{
						Locker->Setup(LockerId(Cell));
						Lockers.Add(Locker);
					}
					break;
				}

				case TEXT('c'): case TEXT('K'): case TEXT('b'): case TEXT('a'):
				case TEXT('M'): case TEXT('n'): case TEXT('s'): case TEXT('x'):
				{
					const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
					FVector Loc = WorldLoc;
					ETTItem Item = ETTItem::Bottle;
					bool bOnTable = true;
					switch (C)
					{
					case TEXT('c'): Item = ETTItem::Crowbar; bOnTable = false; break;
					case TEXT('a'): Item = ETTItem::Bandage; break;
					case TEXT('M'): Item = ETTItem::Medkit; break;
					case TEXT('n'): Item = ETTItem::Magazine; break;
					case TEXT('s'): Item = ETTItem::Smoke; break;
					case TEXT('x'): Item = ETTItem::Grenade; break;
					default: bOnTable = false; break;
					}
					if (C == TEXT('b'))
					{
						Loc += FVector(Rng.FRandRange(-80.f, 80.f), Rng.FRandRange(-80.f, 80.f), 0.f);
					}
					if (ATTPickup* Pickup = World->SpawnActor<ATTPickup>(ATTPickup::StaticClass(), Loc, Rot, Params))
					{
						if (C == TEXT('K'))
						{
							Pickup->Setup(ETTPickupType::Key);
						}
						else
						{
							Pickup->Setup(ETTPickupType::Item, Item, bOnTable);
						}
					}
					break;
				}

				case TEXT('g'):
				{
					GlassCells.Add(Cell);
					for (int32 i = 0; i < 22; ++i)
					{
						const FVector Shard = Local + FVector(Rng.FRandRange(-130.f, 130.f), Rng.FRandRange(-130.f, 130.f), 0.6f);
						const FVector Size(Rng.FRandRange(4.f, 14.f), Rng.FRandRange(3.f, 9.f), 0.8f);
						TTShapes::AddPart(this, RootComponent, Cube, Shard, Size, FLinearColor(0.55f, 0.62f, 0.6f), FRotator(0.f, Rng.FRandRange(0.f, 180.f), 0.f), false);
					}
					break;
				}

				case TEXT('W'):
				case TEXT('X'):
				case TEXT('E'):
				case TEXT('='):
				{
					FIntPoint Inside = Cell;
					float Yaw = 0.f;
					for (const FIntPoint& D : Dirs)
					{
						if (IsWalkableChar(GetChar(Cell + D)))
						{
							Inside = Cell + D;
							Yaw = FVector(static_cast<float>(D.X), static_cast<float>(D.Y), 0.f).Rotation().Yaw;
						}
					}
					const FRotator Rot(0.f, Yaw, 0.f);
					if (C == TEXT('W'))
					{
						if (ATTWindow* Window = World->SpawnActor<ATTWindow>(ATTWindow::StaticClass(), WorldLoc, Rot, Params))
						{
							Window->Setup(CellSize, WallHeight, CellToWorld(Inside));
						}
					}
					else if (C == TEXT('X'))
					{
						if (ATTExitGate* Gate = World->SpawnActor<ATTExitGate>(ATTExitGate::StaticClass(), WorldLoc, Rot, Params))
						{
							Gate->Setup(CellSize, WallHeight);
						}
					}
					else if (C == TEXT('E'))
					{
						Walls->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, WallHeight * 0.5f), FVector(CellSize, CellSize, WallHeight) / 100.f));
						if (ATTSign* Lift = World->SpawnActor<ATTSign>(ATTSign::StaticClass(), WorldLoc + Rot.Vector() * (CellSize * 0.5f + 2.f), Rot, Params))
						{
							Lift->SetupElevator();
						}
					}
					else
					{
						if (ATTCameraConsole* Console = World->SpawnActor<ATTCameraConsole>(ATTCameraConsole::StaticClass(), WorldLoc, Rot, Params))
						{
							Console->Setup();
						}
					}
					break;
				}

				case TEXT('A'): case TEXT('B'): case TEXT('C'): case TEXT('N'): case TEXT('O'):
				case TEXT('Z'): case TEXT('G'): case TEXT('q'): case TEXT('R'): case TEXT('Q'):
				case TEXT('T'): case TEXT('!'):
				{
					ETTStation Kind = ETTStation::ClueA;
					switch (C)
					{
					case TEXT('B'): Kind = ETTStation::ClueB; break;
					case TEXT('C'): Kind = ETTStation::ClueC; break;
					case TEXT('N'): Kind = ETTStation::GuardLog; break;
					case TEXT('O'): Kind = ETTStation::SecurityNote; break;
					case TEXT('Z'): Kind = ETTStation::Safe; break;
					case TEXT('G'): Kind = ETTStation::Generator; break;
					case TEXT('q'): Kind = ETTStation::GateControl; break;
					case TEXT('R'): Kind = ETTStation::Radio; break;
					case TEXT('Q'): Kind = ETTStation::Interference; break;
					case TEXT('T'): Kind = ETTStation::Helipad; break;
					case TEXT('!'): Kind = ETTStation::TunnelExit; break;
					default: break;
					}
					float Yaw = FacingYawAwayFromWall(Cell);
					for (const FIntPoint& D : Dirs)
					{
						if (IsWalkableChar(GetChar(Cell + D)))
						{
							Yaw = FVector(static_cast<float>(D.X), static_cast<float>(D.Y), 0.f).Rotation().Yaw;
							break;
						}
					}
					if (ATTStation* Station = World->SpawnActor<ATTStation>(ATTStation::StaticClass(), WorldLoc, FRotator(0.f, Yaw, 0.f), Params))
					{
						Station->Setup(Kind);
					}
					break;
				}

				case TEXT('F'):
				{
					if (ATTPickup* Pickup = World->SpawnActor<ATTPickup>(ATTPickup::StaticClass(), WorldLoc, FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Params))
					{
						Pickup->Setup(ETTPickupType::Item, ETTItem::Batteries, true);
					}
					break;
				}

				case TEXT('%'):
				{
					TunnelLadders.Add(Cell);
					if (ATTHatch* Hatch = World->SpawnActor<ATTHatch>(ATTHatch::StaticClass(), WorldLoc, FRotator(0.f, FacingYawAwayFromWall(Cell), 0.f), Params))
					{
						Hatch->Setup(C, F, X, Y);
					}
					break;
				}

				case TEXT('V'):
				case TEXT('v'):
				{
					if (ATTHatch* Hatch = World->SpawnActor<ATTHatch>(ATTHatch::StaticClass(), WorldLoc, FRotator(0.f, FacingYawAwayFromWall(Cell), 0.f), Params))
					{
						Hatch->Setup(C, F, X, Y);
					}
					break;
				}

				case TEXT('@'):
				{
					// Ceiling camera looking down the longest open line from this cell.
					FIntPoint BestDir(1, 0);
					int32 BestRun = -1;
					for (const FIntPoint& D : Dirs)
					{
						int32 Run = 0;
						while (Run < 12 && IsWalkableChar(GetChar(Cell + D * (Run + 1))))
						{
							++Run;
						}
						if (Run > BestRun)
						{
							BestRun = Run;
							BestDir = D;
						}
					}
					const FVector Mount = WorldLoc + FVector(-BestDir.X * 120.f, -BestDir.Y * 120.f, (bOutdoor ? 300.f : WallHeight - 30.f));
					const FRotator Look = FRotator(-22.f, FVector(static_cast<float>(BestDir.X), static_cast<float>(BestDir.Y), 0.f).Rotation().Yaw, 0.f);
					TTShapes::AddPart(this, RootComponent, Cube, Mount - GetActorLocation(), FVector(26.f, 14.f, 14.f), FLinearColor(0.1f, 0.1f, 0.1f), Look, false);
					if (ACameraActor* Cam = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Mount, Look, Params))
					{
						UCameraComponent* CamComp = Cam->GetCameraComponent();
						CamComp->SetFieldOfView(80.f);
						// Old security camera with IR: brighter, monochrome, a little green.
						FPostProcessSettings& PP = CamComp->PostProcessSettings;
						CamComp->PostProcessBlendWeight = 1.f;
						PP.bOverride_AutoExposureBias = true;
						PP.AutoExposureBias = 4.5f;
						PP.bOverride_ColorSaturation = true;
						PP.ColorSaturation = FVector4(0.f, 0.f, 0.f, 1.f);
						PP.bOverride_ColorGain = true;
						PP.ColorGain = FVector4(0.85f, 1.f, 0.85f, 1.f);
						PP.bOverride_FilmGrainIntensity = true;
						PP.FilmGrainIntensity = 0.8f;
						PP.bOverride_VignetteIntensity = true;
						PP.VignetteIntensity = 1.f;
						SecurityCameras.Add(Cam);
						SecurityCameraNames.Add(FString::Printf(TEXT("CAM %02d  %s"), ++CameraIndex, *GetFloorLabel(F)));
					}
					break;
				}

				case TEXT('m'):
				{
					const float Yaw = FacingYawAwayFromWall(Cell) + 90.f;
					const FRotator Rot(0.f, Yaw, 0.f);
					const FVector Fwd = Rot.Vector();
					const FVector Right = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y);
					const FVector Base = Local + Rng.FRandRange(-20.f, 20.f) * Fwd;
					TTShapes::AddPart(this, RootComponent, Cube, Base + FVector(0.f, 0.f, 55.f), FVector(205.f, 95.f, 14.f), BedColor, Rot);
					TTShapes::AddPart(this, RootComponent, Cube, Base + FVector(0.f, 0.f, 68.f), FVector(195.f, 88.f, 12.f), SheetColor, Rot + FRotator(0.f, Rng.FRandRange(-3.f, 3.f), 0.f));
					TTShapes::AddPart(this, RootComponent, Cube, Base + Fwd * 95.f + FVector(0.f, 0.f, 75.f), FVector(6.f, 95.f, 80.f), BedColor, Rot);
					for (int32 i = 0; i < 4; ++i)
					{
						const FVector Leg = Base + Fwd * ((i < 2) ? 92.f : -92.f) + Right * ((i % 2) ? 40.f : -40.f) + FVector(0.f, 0.f, 25.f);
						TTShapes::AddPart(this, RootComponent, Cube, Leg, FVector(5.f, 5.f, 50.f), BedColor, Rot, false);
					}
					break;
				}

				case TEXT('t'):
				{
					const float Yaw = FacingYawAwayFromWall(Cell);
					const FRotator Rot(0.f, Yaw + 90.f, 0.f);
					if (Rng.FRand() < 0.4f)
					{
						TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 100.f), FVector(200.f, 55.f, 200.f), FLinearColor(0.22f, 0.22f, 0.2f), Rot);
					}
					else
					{
						TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 38.f), FVector(160.f, 85.f, 76.f), DeskColor, Rot);
						TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 79.f), FVector(40.f, 30.f, 4.f), FLinearColor(0.75f, 0.73f, 0.65f), Rot + FRotator(0.f, 20.f, 0.f), false);
					}
					break;
				}

				case TEXT('r'):
				{
					const FLinearColor Paint(Rng.FRandRange(0.05f, 0.3f), Rng.FRandRange(0.05f, 0.2f), Rng.FRandRange(0.05f, 0.2f));
					const FRotator Rot(0.f, Rng.FRandRange(-6.f, 6.f) + (Rng.FRand() < 0.5f ? 0.f : 180.f), 0.f);
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 55.f), FVector(285.f, 170.f, 75.f), Paint, Rot);
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(-15.f, 0.f, 120.f), FVector(150.f, 150.f, 60.f), Paint * 0.7f, Rot);
					break;
				}

				case TEXT('h'):
				{
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 95.f), FVector(285.f, 285.f, 190.f), FLinearColor(0.25f, 0.26f, 0.25f));
					TTShapes::AddPart(this, RootComponent, TTShapes::Cylinder(), Local + FVector(0.f, 0.f, 200.f), FVector(160.f, 160.f, 20.f), FLinearColor(0.1f, 0.1f, 0.1f), FRotator::ZeroRotator, false);
					break;
				}

				case TEXT('d'): case TEXT('e'): case TEXT('k'): case TEXT('j'):
				{
					FStoryCell Story;
					Story.Char = C;
					Story.Cell = Cell;
					for (const FDocSource& Doc : DocSources)
					{
						if (Floors[F].Id == Doc.Floor && Doc.X == X && Doc.Y == Y)
						{
							Story.DocId = Doc.Id;
						}
					}
					StoryCells.Add(Story);
					break;
				}

				case TEXT('o'):
				{
					// Morgue slab: steel table. Whether a body lies on it depends on the difficulty (narrative).
					const FRotator Rot(0.f, FacingYawAwayFromWall(Cell) + 90.f, 0.f);
					const FLinearColor Steel(0.32f, 0.34f, 0.35f);
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 80.f), FVector(200.f, 80.f, 8.f), Steel, Rot);
					TTShapes::AddPart(this, RootComponent, Cube, Local + FVector(0.f, 0.f, 40.f), FVector(20.f, 20.f, 76.f), Steel * 0.7f, Rot);
					FStoryCell Story;
					Story.Char = C;
					Story.Cell = Cell;
					StoryCells.Add(Story);
					break;
				}

				case TEXT('P'):
					PlayerStart = Cell;
					break;
				case TEXT('H'):
					HunterStart = Cell;
					break;
				case TEXT('Y'):
					CompanionStarts.Add(0, Cell);
					break;
				case TEXT('U'):
					CompanionStarts.Add(1, Cell);
					break;
				case TEXT('J'):
					CompanionStarts.Add(2, Cell);
					break;

				default:
					if (C >= TEXT('1') && C <= TEXT('9'))
					{
						PatrolPoints.Add(Cell);
					}
					break;
				}
			}
		}

		BuildStairs(F);
	}

	// A couple of security cameras show nothing but noise (GDD §26).
	if (SecurityCameras.Num() > 2)
	{
		BrokenCameras.Add(Rng.RandRange(0, SecurityCameras.Num() - 1));
		BrokenCameras.Add(Rng.RandRange(0, SecurityCameras.Num() - 1));
	}

	// Joe faces the first step out of the room.
	int32 BestLen = TNumericLimits<int32>::Max();
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Doors)
	{
		if (FloorOfCell(Pair.Key) != FloorOfCell(PlayerStart))
		{
			continue;
		}
		TArray<FIntPoint> Route;
		if (FindPath(PlayerStart, Pair.Key, Route) && Route.Num() > 1 && Route.Num() < BestLen)
		{
			BestLen = Route.Num();
			const FIntPoint Target = Route[FMath::Min(2, Route.Num() - 1)];
			const FVector Delta(static_cast<float>(Target.X - PlayerStart.X), static_cast<float>(Target.Y - PlayerStart.Y), 0.f);
			PlayerStartYaw = Delta.Rotation().Yaw;
		}
	}

	SpawnAtmosphere();
	SpawnCity();
	UE_LOG(LogTemp, Log, TEXT("[TT] Hospital built: %d floors, %d doors, %d lockers, %d lamps, %d cameras, %d patrol points"),
		Floors.Num(), Doors.Num(), Lockers.Num(), Lamps.Num(), SecurityCameras.Num(), PatrolPoints.Num());
}

FString ATTHospital::GetZoneOfCell(const FIntPoint& Cell) const
{
	if (Floors.Num() == 0)
	{
		return FString();
	}
	const int32 F = FloorOfCell(Cell);
	const int32 X = Cell.X - F * Stride;
	const int32 Y = Cell.Y;
	for (const FZoneSource& Zone : ZoneSources)
	{
		if (Floors[F].Id == Zone.Floor && X >= Zone.X0 && X <= Zone.X1 && Y >= Zone.Y0 && Y <= Zone.Y1)
		{
			return Zone.Node;
		}
	}
	return FString();
}

FString ATTHospital::GetZoneAt(const FVector& Location) const
{
	return GetZoneOfCell(WorldToCell(Location));
}

float ATTHospital::FacingYawForDevice(const FIntPoint& Cell) const
{
	for (const FIntPoint& D : Dirs)
	{
		if (IsWalkableChar(GetChar(Cell + D)))
		{
			return FVector(static_cast<float>(D.X), static_cast<float>(D.Y), 0.f).Rotation().Yaw;
		}
	}
	return FacingYawAwayFromWall(Cell);
}

void ATTHospital::BuildStairs(int32 F)
{
	if (!Floors[F].bStairs)
	{
		return;
	}
	UStaticMesh* Cube = TTShapes::Cube();
	const float Z0 = FloorZ(F);
	const bool bTop = !(Floors.IsValidIndex(F + 1) && Floors[F + 1].bStairs);
	const bool bEven = F % 2 == 0;
	for (const FIntPoint& Def : StairDefs)
	{
		const int32 A = Def.X;
		const int32 B = Def.X + 1;
		const int32 R0 = Def.Y;
		const int32 R1 = R0 + 1;
		const int32 R2 = R0 + 2;

		// Rail between the two lanes, the whole height of the floor.
		const FVector RailLocal((A + 0.5f) * CellSize, (R1 + 0.5f) * CellSize, Z0 + WallHeight * 0.5f);
		TTShapes::AddPart(this, RootComponent, Cube, RailLocal, FVector(10.f, 2.f * CellSize, WallHeight), FLinearColor(0.25f, 0.25f, 0.24f));

		if (bTop)
		{
			continue;
		}
		const int32 Lane = bEven ? A : B;
		FVector P0;
		FVector P1;
		if (bEven)
		{
			// North-going flight: from the south edge of R2 to the north edge of R1.
			P0 = FVector(Lane * CellSize, R2 * CellSize + CellSize * 0.5f, Z0);
			P1 = FVector(Lane * CellSize, R1 * CellSize - CellSize * 0.5f, Z0 + FloorHeight);
			AddLink(FIntPoint(F * Stride + Lane, R1), FIntPoint((F + 1) * Stride + Lane, R0));
		}
		else
		{
			P0 = FVector(Lane * CellSize, R1 * CellSize - CellSize * 0.5f, Z0);
			P1 = FVector(Lane * CellSize, R2 * CellSize + CellSize * 0.5f, Z0 + FloorHeight);
			AddLink(FIntPoint(F * Stride + Lane, R2), FIntPoint((F + 1) * Stride + Lane, R0 + 3));
		}
		const FVector Dir = P1 - P0;
		const FRotator Rot = Dir.Rotation();
		const FVector Up = FRotationMatrix(Rot).GetUnitAxis(EAxis::Z);
		const FVector Center = (P0 + P1) * 0.5f - Up * 8.f;
		TTShapes::AddPart(this, RootComponent, Cube, Center, FVector(Dir.Size() + 10.f, CellSize - 30.f, 16.f), FLinearColor(0.3f, 0.3f, 0.29f), Rot);
		// Step nosings, so it reads as a staircase.
		for (int32 i = 1; i < 14; ++i)
		{
			const FVector P = P0 + Dir * (i / 14.f) + Up * 1.f;
			TTShapes::AddPart(this, RootComponent, Cube, P, FVector(6.f, CellSize - 40.f, 2.f), FLinearColor(0.12f, 0.12f, 0.12f), Rot, false);
		}
		// A dim lamp on every landing.
		AddLamp(FIntPoint(F * Stride + (bEven ? B : A), bEven ? R0 + 3 : R0), false);
	}
}

void ATTHospital::AddLamp(const FIntPoint& Cell, bool bFlicker)
{
	const int32 Floor = FloorOfCell(Cell);
	const FVector Local = CellToWorld(Cell) - GetActorLocation();

	FLamp Lamp;
	Lamp.Floor = Floor;
	Lamp.Fixture = TTShapes::AddPart(this, RootComponent, TTShapes::Cube(), Local + FVector(0.f, 0.f, WallHeight - 3.f), FVector(130.f, 32.f, 6.f), FLinearColor(0.9f, 0.9f, 0.85f), FRotator::ZeroRotator, false);
	if (Lamp.Fixture)
	{
		Lamp.Glow = TTShapes::MakeEmissive(Lamp.Fixture, bFlicker ? FLinearColor(0.8f, 0.92f, 1.f) : FLinearColor(1.f, 0.93f, 0.8f), 6.f);
		Lamp.Fixture->SetMaterial(0, Lamp.Glow);
	}

	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(Local + FVector(0.f, 0.f, WallHeight - 25.f));
	Light->SetIntensityUnits(ELightUnits::Lumens);
	Lamp.BaseIntensity = bFlicker ? 380.f : 520.f;
	Light->SetIntensity(Lamp.BaseIntensity);
	Light->SetAttenuationRadius(820.f);
	Light->SetSourceRadius(20.f);
	Light->SetLightColor(bFlicker ? FLinearColor(0.85f, 0.95f, 1.f) : FLinearColor(1.f, 0.93f, 0.8f));
	Light->SetVolumetricScatteringIntensity(1.5f);
	Light->SetCastShadows(true);
	Light->RegisterComponent();

	Lamp.Light = Light;
	Lamp.bFlicker = bFlicker;
	Lamps.Add(Lamp);
}

void ATTHospital::SpawnAtmosphere()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;

	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), GetActorLocation() + FVector(0.f, 0.f, FloorZ(0)), FRotator::ZeroRotator, Params))
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		FogComp->SetMobility(EComponentMobility::Movable);
		FogComp->SetFogDensity(0.03f);
		FogComp->SetFogHeightFalloff(0.001f);
		FogComp->SetFogInscatteringColor(FLinearColor(0.002f, 0.0025f, 0.003f));
		FogComp->SetVolumetricFog(true);
		FogComp->SetVolumetricFogScatteringDistribution(0.5f);
		FogComp->SetVolumetricFogExtinctionScale(1.5f);
	}

	// Moonlight: only the roof and the stairwell openings see it.
	if (ADirectionalLight* Moon = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), GetActorLocation(), FRotator(-38.f, 35.f, 0.f), Params))
	{
		ULightComponent* MoonLight = Moon->GetLightComponent();
		MoonLight->SetMobility(EComponentMobility::Movable);
		MoonLight->SetIntensity(1.2f);
		MoonLight->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
	}

	float ExposureBias = -1.2f;
	FParse::Value(FCommandLine::Get(), TEXT("ttev="), ExposureBias);

	if (APostProcessVolume* Post = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Post->bUnbound = true;
		FPostProcessSettings& S = Post->Settings;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = 0;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = ExposureBias;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.75f;
		S.bOverride_FilmGrainIntensity = true;
		S.FilmGrainIntensity = 0.35f;
		S.bOverride_SceneFringeIntensity = true;
		S.SceneFringeIntensity = 0.6f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.5f;
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(0.8f, 0.85f, 0.85f, 1.f);
		S.bOverride_ColorGamma = true;
		S.ColorGamma = FVector4(1.f, 1.f, 1.02f, 1.f);
	}
}

void ATTHospital::SpawnCity()
{
	// The city at night around the hospital: dark blocks with a few lit windows.
	FRandomStream Rng(777);
	UStaticMesh* Cube = TTShapes::Cube();
	const FVector Center(FloorW * CellSize * 0.5f, FloorH * CellSize * 0.5f, 0.f);
	for (int32 i = 0; i < 46; ++i)
	{
		const float Angle = Rng.FRandRange(0.f, 2.f * PI);
		const float Dist = Rng.FRandRange(6500.f, 15000.f);
		const FVector Pos = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
		FVector Size(Rng.FRandRange(900.f, 2400.f), Rng.FRandRange(900.f, 2400.f), Rng.FRandRange(1800.f, 7000.f));
		if (FMath::Abs(FMath::UnwindRadians(Angle)) < 0.35f)
		{
			Size.Z = FMath::Min(Size.Z, 1500.f);
		}
		TTShapes::AddPart(this, RootComponent, Cube, Pos + FVector(0.f, 0.f, FloorZ(0) + Size.Z * 0.5f - 400.f), Size, FLinearColor(0.012f, 0.013f, 0.016f), FRotator(0.f, Rng.FRandRange(0.f, 90.f), 0.f), false);
		const int32 Lit = Rng.RandRange(2, 7);
		for (int32 k = 0; k < Lit; ++k)
		{
			const FVector WindowPos = Pos + FVector(Rng.FRandRange(-0.5f, 0.5f) * Size.X, Rng.FRandRange(-0.5f, 0.5f) * Size.Y, FloorZ(0) + Rng.FRandRange(0.f, Size.Z - 600.f));
			if (UStaticMeshComponent* Glow = TTShapes::AddPart(this, RootComponent, Cube, WindowPos, FVector(140.f, 140.f, 90.f), FLinearColor::Black, FRotator::ZeroRotator, false))
			{
				Glow->SetMaterial(0, TTShapes::MakeEmissive(Glow, FLinearColor(1.f, 0.75f, 0.4f), Rng.FRandRange(1.f, 3.f)));
				Glow->SetCastShadow(false);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Tick: flickering lamps, and only the floors around Joe keep their lights on.

void ATTHospital::UpdateLampCulling()
{
	// Follows the view: Joe's eyes, or the security camera he is watching.
	const APlayerCameraManager* View = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!View)
	{
		return;
	}
	const int32 JoeFloor = FloorOfLocation(View->GetCameraLocation() - FVector(0.f, 0.f, 150.f));
	if (JoeFloor == LastCullFloor)
	{
		return;
	}
	LastCullFloor = JoeFloor;
	for (FLamp& Lamp : Lamps)
	{
		Lamp.bActive = FMath::Abs(Lamp.Floor - JoeFloor) <= 1;
		if (Lamp.Light)
		{
			Lamp.Light->SetVisibility(Lamp.bActive);
		}
	}
}

void ATTHospital::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CullTimer -= DeltaSeconds;
	if (CullTimer <= 0.f)
	{
		CullTimer = 0.4f;
		UpdateLampCulling();
	}
	for (FLamp& Lamp : Lamps)
	{
		if (!Lamp.bFlicker || !Lamp.Light || Lamp.bKilled || !Lamp.bActive || (Lamp.bNeedsPower && !bPowered))
		{
			continue;
		}
		Lamp.Timer -= DeltaSeconds;
		if (Lamp.Timer <= 0.f)
		{
			Lamp.bOn = !Lamp.bOn;
			Lamp.Timer = Lamp.bOn ? FMath::FRandRange(0.05f, 1.8f) : FMath::FRandRange(0.03f, 0.35f);
			Lamp.Light->SetIntensity(Lamp.bOn ? Lamp.BaseIntensity * FMath::FRandRange(0.7f, 1.f) : 0.f);
			if (Lamp.Glow)
			{
				Lamp.Glow->SetScalarParameterValue(TEXT("Intensity"), Lamp.bOn ? 6.f : 0.05f);
			}
		}
	}
}

float ATTHospital::GetLightLevelAt(const FVector& Location, const AActor* IgnoreActor) const
{
	const float Radius = 620.f;
	const int32 Floor = FloorOfLocation(Location);
	float Level = 0.f;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTLightLevel), false);
	if (IgnoreActor)
	{
		Query.AddIgnoredActor(IgnoreActor);
	}
	for (const FLamp& Lamp : Lamps)
	{
		if (!Lamp.bOn || !Lamp.Light || Lamp.Floor != Floor)
		{
			continue;
		}
		const FVector LampLoc = Lamp.Light->GetComponentLocation();
		const float Dist = FVector::Dist(LampLoc, Location);
		if (Dist >= Radius)
		{
			continue;
		}
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, LampLoc, Location, ECC_WorldStatic, Query))
		{
			continue;
		}
		Level = FMath::Max(Level, 1.f - Dist / Radius);
	}
	// Moonlit roof.
	if (Floors.IsValidIndex(Floor) && Floors[Floor].bOutdoor)
	{
		Level = FMath::Max(Level, 0.35f);
	}
	return Level;
}

void ATTHospital::SetPowered(bool bInPowered)
{
	bPowered = bInPowered;
	for (FLamp& Lamp : Lamps)
	{
		if (!Lamp.bNeedsPower || Lamp.bKilled || !Lamp.Light)
		{
			continue;
		}
		Lamp.bOn = bPowered;
		Lamp.Light->SetIntensity(bPowered ? Lamp.BaseIntensity : 0.f);
		if (Lamp.Glow)
		{
			Lamp.Glow->SetScalarParameterValue(TEXT("Intensity"), bPowered ? 6.f : 0.05f);
		}
	}
}

void ATTHospital::EnableTunnelLinks()
{
	if (bTunnelLinks)
	{
		return;
	}
	bTunnelLinks = true;
	for (const FIntPoint& A : TunnelLadders)
	{
		for (const FIntPoint& B : TunnelLadders)
		{
			const int32 LA = A.X - FloorOfCell(A) * Stride;
			const int32 LB = B.X - FloorOfCell(B) * Stride;
			if (A != B && LA == LB && A.Y == B.Y && FloorOfCell(A) < FloorOfCell(B))
			{
				AddLink(A, B);
			}
		}
	}
}

void ATTHospital::BreakAllCameras()
{
	for (int32 i = 0; i < SecurityCameras.Num(); ++i)
	{
		BrokenCameras.Add(i);
	}
}

int32 ATTHospital::SetLampsNear(const FVector& Center, float Radius, bool bOn)
{
	const int32 Floor = FloorOfLocation(Center);
	int32 Changed = 0;
	for (FLamp& Lamp : Lamps)
	{
		if (!Lamp.Light || Lamp.bKilled == !bOn || Lamp.Floor != Floor)
		{
			continue;
		}
		const FVector LampLoc = Lamp.Light->GetComponentLocation();
		if (FVector::Dist2D(LampLoc, Center) > Radius)
		{
			continue;
		}
		Lamp.bKilled = !bOn;
		Lamp.bOn = bOn;
		Lamp.Light->SetIntensity(bOn ? Lamp.BaseIntensity : 0.f);
		if (Lamp.Glow)
		{
			Lamp.Glow->SetScalarParameterValue(TEXT("Intensity"), bOn ? 6.f : 0.05f);
		}
		TTAudio::PlayAt(this, ETTSfx::Click, LampLoc, 0.9f);
		++Changed;
	}
	return Changed;
}

// ---------------------------------------------------------------------------
// Navigation

ATTDoor* ATTHospital::GetDoorAt(const FIntPoint& Cell) const
{
	const TObjectPtr<ATTDoor>* Found = Doors.Find(Cell);
	return Found ? Found->Get() : nullptr;
}

bool ATTHospital::HasLockedDoor() const
{
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Doors)
	{
		if (Pair.Value && Pair.Value->IsLocked())
		{
			return true;
		}
	}
	return false;
}

bool ATTHospital::IsConnectedWithout(const FIntPoint& Blocked) const
{
	auto Reachable = [this](const FIntPoint& Skip)
	{
		TArray<bool> Seen;
		Seen.Init(false, Walkable.Num());
		TArray<FIntPoint> Queue;
		Queue.Add(PlayerStart);
		Seen[PlayerStart.Y * GridWidth + PlayerStart.X] = true;
		TArray<FIntPoint> Next;
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FIntPoint C = Queue[Head];
			Next.Reset();
			for (const FIntPoint& D : Dirs)
			{
				Next.Add(C + D);
			}
			Links.MultiFind(C, Next);
			for (const FIntPoint& N : Next)
			{
				if (N == Skip || !IsWalkable(N) || Seen[N.Y * GridWidth + N.X])
				{
					continue;
				}
				Seen[N.Y * GridWidth + N.X] = true;
				Queue.Add(N);
			}
		}
		return Queue.Num();
	};
	// Blocking a cell may only remove that cell from what Joe can reach right now.
	return Reachable(Blocked) == Reachable(FIntPoint(-1, -1)) - (IsWalkable(Blocked) ? 1 : 0);
}

FIntPoint ATTHospital::FindNearestWalkable(const FIntPoint& Cell) const
{
	if (IsWalkable(Cell))
	{
		return Cell;
	}
	const int32 Floor = FloorOfCell(Cell);
	for (int32 R = 1; R <= 4; ++R)
	{
		for (int32 Y = -R; Y <= R; ++Y)
		{
			for (int32 X = -R; X <= R; ++X)
			{
				const FIntPoint C = Cell + FIntPoint(X, Y);
				if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) == R && FloorOfCell(C) == Floor && IsWalkable(C))
				{
					return C;
				}
			}
		}
	}
	return HunterStart;
}

TArray<FIntPoint> ATTHospital::GetWalkableCellsNear(const FIntPoint& Center, int32 Radius) const
{
	TArray<FIntPoint> Result;
	const int32 Floor = FloorOfCell(Center);
	for (int32 Y = -Radius; Y <= Radius; ++Y)
	{
		for (int32 X = -Radius; X <= Radius; ++X)
		{
			const FIntPoint C = Center + FIntPoint(X, Y);
			if (FloorOfCell(C) == Floor && IsWalkable(C) && !Doors.Contains(C) && StairCellAt(Floor, C.X - Floor * Stride, C.Y) == EStairCell::None)
			{
				Result.Add(C);
			}
		}
	}
	return Result;
}

bool ATTHospital::FindPath(const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPath, bool bForHunter) const
{
	OutPath.Reset();
	const FIntPoint Start = FindNearestWalkable(From);
	const FIntPoint Goal = FindNearestWalkable(To);
	if (Start == Goal)
	{
		OutPath.Add(Goal);
		return true;
	}
	const bool bAvoidIsolation = bForHunter && !bHunterKnowsIsolation && !IsolationCells.Contains(Goal);

	const int32 Count = Walkable.Num();
	TArray<float> G;
	G.Init(TNumericLimits<float>::Max(), Count);
	TArray<int32> Parent;
	Parent.Init(INDEX_NONE, Count);
	TArray<bool> Closed;
	Closed.Init(false, Count);
	TArray<int32> Open;

	auto Index = [this](const FIntPoint& P) { return P.Y * GridWidth + P.X; };
	const int32 GoalFloor = FloorOfCell(Goal);
	const int32 GoalLocalX = Goal.X - GoalFloor * Stride;
	auto Heuristic = [&](const FIntPoint& P)
	{
		const int32 PF = FloorOfCell(P);
		const int32 LX = P.X - PF * Stride;
		return static_cast<float>(FMath::Abs(LX - GoalLocalX) + FMath::Abs(P.Y - Goal.Y) + 4 * FMath::Abs(PF - GoalFloor));
	};

	G[Index(Start)] = 0.f;
	Open.Add(Index(Start));
	TArray<FIntPoint> Next;

	while (Open.Num() > 0)
	{
		int32 BestSlot = 0;
		float BestF = TNumericLimits<float>::Max();
		for (int32 i = 0; i < Open.Num(); ++i)
		{
			const FIntPoint P(Open[i] % GridWidth, Open[i] / GridWidth);
			const float F = G[Open[i]] + Heuristic(P);
			if (F < BestF)
			{
				BestF = F;
				BestSlot = i;
			}
		}
		const int32 Current = Open[BestSlot];
		Open.RemoveAtSwap(BestSlot);
		if (Closed[Current])
		{
			continue;
		}
		Closed[Current] = true;

		const FIntPoint CP(Current % GridWidth, Current / GridWidth);
		if (CP == Goal)
		{
			for (int32 Node = Current; Node != INDEX_NONE; Node = Parent[Node])
			{
				OutPath.Insert(FIntPoint(Node % GridWidth, Node / GridWidth), 0);
			}
			return true;
		}

		Next.Reset();
		for (const FIntPoint& D : Dirs)
		{
			Next.Add(CP + D);
		}
		Links.MultiFind(CP, Next);
		for (const FIntPoint& NP : Next)
		{
			if (!IsWalkable(NP) || (bAvoidIsolation && IsolationCells.Contains(NP)))
			{
				continue;
			}
			const int32 NI = Index(NP);
			const float Cost = G[Current] + (FloorOfCell(NP) != FloorOfCell(CP) ? 3.f : 1.f);
			if (!Closed[NI] && Cost < G[NI])
			{
				G[NI] = Cost;
				Parent[NI] = Current;
				Open.Add(NI);
			}
		}
	}
	return false;
}
