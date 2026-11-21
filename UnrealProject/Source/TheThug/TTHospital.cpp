#include "TTHospital.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TTAudio.h"
#include "TTInteractables.h"
#include "TTShapes.h"

namespace
{
	/**
	 * 4th floor, west wing (M01 slice).
	 *  #  wall            .  floor            D  door            L  locker
	 *  *  ceiling lamp    ~  flickering lamp  m  hospital bed    b  bottle
	 *  P  player start    H  Hunter start     K  gate keycard    X  service gate (exit)
	 *  1-9  Hunter patrol points
	 */
	const TCHAR* const Layout[] = {
		TEXT("#######################"),
		TEXT("#L..m#m..m#...L#m....L#"),
		TEXT("#....#.1..D.2..#..~.P.#"),
		TEXT("#L*..#..*L#m...#L....m#"),
		TEXT("##D#####D####D#####D###"),
		TEXT("#.*..3.b.~.4..*..5..*.X"),
		TEXT("###D####D####D####D####"),
		TEXT("#m..L#L..m#..*L#....mL#"),
		TEXT("#.6..D....D.H..#.7.~..#"),
		TEXT("#~.Km#.*.b#L..m#m....L#"),
		TEXT("#######################"),
	};

	const FLinearColor WallColor(0.34f, 0.37f, 0.33f);
	const FLinearColor TrimColor(0.12f, 0.17f, 0.15f);
	const FLinearColor FloorColor(0.16f, 0.16f, 0.15f);
	const FLinearColor CeilingColor(0.22f, 0.22f, 0.21f);
	const FLinearColor BedColor(0.30f, 0.31f, 0.30f);
	const FLinearColor SheetColor(0.42f, 0.42f, 0.38f);

	const FIntPoint Dirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

	bool IsWalkableChar(TCHAR C)
	{
		switch (C)
		{
		case TEXT('.'): case TEXT('*'): case TEXT('~'): case TEXT('D'): case TEXT('L'):
		case TEXT('P'): case TEXT('H'): case TEXT('b'):
			return true;
		default:
			return C >= TEXT('1') && C <= TEXT('9');
		}
	}
}

ATTHospital::ATTHospital()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

TCHAR ATTHospital::GetChar(int32 X, int32 Y) const
{
	if (Y < 0 || Y >= Height || X < 0 || X >= Rows[Y].Len())
	{
		return TEXT('#');
	}
	return Rows[Y][X];
}

FVector ATTHospital::CellToWorld(const FIntPoint& Cell, float Z) const
{
	return GetActorLocation() + FVector(Cell.X * CellSize, Cell.Y * CellSize, Z);
}

FIntPoint ATTHospital::WorldToCell(const FVector& Location) const
{
	const FVector Local = Location - GetActorLocation();
	return FIntPoint(FMath::RoundToInt(Local.X / CellSize), FMath::RoundToInt(Local.Y / CellSize));
}

bool ATTHospital::IsWalkable(const FIntPoint& Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= Width || Cell.Y >= Height)
	{
		return false;
	}
	return Walkable[Cell.Y * Width + Cell.X];
}

float ATTHospital::FacingYawAwayFromWall(const FIntPoint& Cell) const
{
	for (const FIntPoint& D : Dirs)
	{
		const TCHAR Neighbor = GetChar(Cell.X + D.X, Cell.Y + D.Y);
		const TCHAR Opposite = GetChar(Cell.X - D.X, Cell.Y - D.Y);
		if (Neighbor == TEXT('#') && Opposite != TEXT('#'))
		{
			return FVector(static_cast<float>(-D.X), static_cast<float>(-D.Y), 0.f).Rotation().Yaw;
		}
	}
	return 0.f;
}

void ATTHospital::Build()
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;

	for (const TCHAR* Row : Layout)
	{
		Rows.Add(Row);
		Width = FMath::Max(Width, Rows.Last().Len());
	}
	Height = Rows.Num();
	Walkable.Init(false, Width * Height);

	UWorld* World = GetWorld();
	UStaticMesh* Cube = TTShapes::Cube();

	// Floor and ceiling slabs.
	const FVector Center = CellToWorld(FIntPoint(0, 0)) + FVector((Width - 1) * CellSize * 0.5f, (Height - 1) * CellSize * 0.5f, 0.f);
	const FVector Extent(Width * CellSize, Height * CellSize, 20.f);
	TTShapes::AddPart(this, RootComponent, Cube, Center - GetActorLocation() + FVector(0.f, 0.f, -10.f), Extent, FloorColor);
	TTShapes::AddPart(this, RootComponent, Cube, Center - GetActorLocation() + FVector(0.f, 0.f, WallHeight + 10.f), Extent, CeilingColor);

	Walls = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Walls"));
	Walls->SetStaticMesh(Cube);
	Walls->SetupAttachment(RootComponent);
	Walls->SetMaterial(0, TTShapes::MakeColor(Walls, WallColor, 0.9f));
	Walls->RegisterComponent();

	WallTrim = NewObject<UInstancedStaticMeshComponent>(this, TEXT("WallTrim"));
	WallTrim->SetStaticMesh(Cube);
	WallTrim->SetupAttachment(RootComponent);
	WallTrim->SetMaterial(0, TTShapes::MakeColor(WallTrim, TrimColor, 0.5f));
	WallTrim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallTrim->RegisterComponent();

	FRandomStream Rng(4242);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const TCHAR C = GetChar(X, Y);
			const FIntPoint Cell(X, Y);
			const FVector Local(X * CellSize, Y * CellSize, 0.f);
			const FVector WorldLoc = CellToWorld(Cell);
			Walkable[Y * Width + X] = IsWalkableChar(C);

			switch (C)
			{
			case TEXT('#'):
				Walls->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, WallHeight * 0.5f), FVector(CellSize, CellSize, WallHeight) / 100.f));
				WallTrim->AddInstance(FTransform(FRotator::ZeroRotator, Local + FVector(0.f, 0.f, 55.f), FVector(CellSize + 4.f, CellSize + 4.f, 110.f) / 100.f));
				break;

			case TEXT('*'):
				AddLamp(Cell, false, false);
				break;

			case TEXT('~'):
				AddLamp(Cell, true, false);
				break;

			case TEXT('D'):
			{
				const bool bWallsLeftRight = GetChar(X - 1, Y) == TEXT('#') && GetChar(X + 1, Y) == TEXT('#');
				const FRotator Rot(0.f, bWallsLeftRight ? 0.f : 90.f, 0.f);
				if (ATTDoor* Door = World->SpawnActor<ATTDoor>(ATTDoor::StaticClass(), WorldLoc, Rot, Params))
				{
					Door->Setup(CellSize, WallHeight, DoorId(Cell));
					Doors.Add(Cell, Door);
				}
				break;
			}

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

			case TEXT('K'):
			case TEXT('b'):
			{
				const FRotator Rot(0.f, Rng.FRandRange(0.f, 360.f), 0.f);
				FVector Loc = WorldLoc;
				if (C == TEXT('b'))
				{
					Loc += FVector(Rng.FRandRange(-80.f, 80.f), Rng.FRandRange(-80.f, 80.f), 0.f);
				}
				if (ATTPickup* Pickup = World->SpawnActor<ATTPickup>(ATTPickup::StaticClass(), Loc, Rot, Params))
				{
					Pickup->Setup(C == TEXT('K') ? ETTPickupType::Key : ETTPickupType::Bottle);
				}
				break;
			}

			case TEXT('X'):
			{
				float Yaw = 0.f;
				for (const FIntPoint& D : Dirs)
				{
					if (IsWalkableChar(GetChar(X + D.X, Y + D.Y)))
					{
						Yaw = FVector(static_cast<float>(D.X), static_cast<float>(D.Y), 0.f).Rotation().Yaw;
					}
				}
				if (ATTExitGate* Gate = World->SpawnActor<ATTExitGate>(ATTExitGate::StaticClass(), WorldLoc, FRotator(0.f, Yaw, 0.f), Params))
				{
					Gate->Setup(CellSize, WallHeight);
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

			case TEXT('P'):
				PlayerStart = Cell;
				break;

			case TEXT('H'):
				HunterStart = Cell;
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

	// Face the player towards the closest door so the first thing they see is the way out of the room.
	// Uses the first step of the walkable path, so he never starts facing a wall.
	int32 BestLen = TNumericLimits<int32>::Max();
	for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Doors)
	{
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
}

void ATTHospital::AddLamp(const FIntPoint& Cell, bool bFlicker, bool bDead)
{
	const FVector Local(Cell.X * CellSize, Cell.Y * CellSize, 0.f);

	FLamp Lamp;
	Lamp.Fixture = TTShapes::AddPart(this, RootComponent, TTShapes::Cube(), Local + FVector(0.f, 0.f, WallHeight - 3.f), FVector(130.f, 32.f, 6.f), FLinearColor(0.9f, 0.9f, 0.85f), FRotator::ZeroRotator, false);

	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(Local + FVector(0.f, 0.f, WallHeight - 25.f));
	Light->SetIntensityUnits(ELightUnits::Lumens);
	Lamp.BaseIntensity = bFlicker ? 380.f : 520.f;
	Light->SetIntensity(bDead ? 0.f : Lamp.BaseIntensity);
	Light->SetAttenuationRadius(820.f);
	Light->SetSourceRadius(20.f);
	Light->SetLightColor(bFlicker ? FLinearColor(0.85f, 0.95f, 1.f) : FLinearColor(1.f, 0.93f, 0.8f));
	Light->SetVolumetricScatteringIntensity(1.5f);
	Light->SetCastShadows(true);
	Light->RegisterComponent();

	Lamp.Light = Light;
	Lamp.bFlicker = bFlicker;
	Lamp.bOn = !bDead;
	Lamps.Add(Lamp);
}

void ATTHospital::SpawnAtmosphere()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;

	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		FogComp->SetMobility(EComponentMobility::Movable);
		FogComp->SetFogDensity(0.03f);
		FogComp->SetFogHeightFalloff(0.01f);
		FogComp->SetFogInscatteringColor(FLinearColor(0.002f, 0.0025f, 0.003f));
		FogComp->SetVolumetricFog(true);
		FogComp->SetVolumetricFogScatteringDistribution(0.5f);
		FogComp->SetVolumetricFogExtinctionScale(1.5f);
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

void ATTHospital::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	for (FLamp& Lamp : Lamps)
	{
		if (!Lamp.bFlicker || !Lamp.Light || Lamp.bKilled)
		{
			continue;
		}
		Lamp.Timer -= DeltaSeconds;
		if (Lamp.Timer <= 0.f)
		{
			Lamp.bOn = !Lamp.bOn;
			Lamp.Timer = Lamp.bOn ? FMath::FRandRange(0.05f, 1.8f) : FMath::FRandRange(0.03f, 0.35f);
			Lamp.Light->SetIntensity(Lamp.bOn ? Lamp.BaseIntensity * FMath::FRandRange(0.7f, 1.f) : 0.f);
			if (Lamp.Fixture)
			{
				Lamp.Fixture->SetVisibility(true);
			}
		}
	}
}

float ATTHospital::GetLightLevelAt(const FVector& Location, const AActor* IgnoreActor) const
{
	const float Radius = 620.f;
	float Level = 0.f;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTLightLevel), false);
	if (IgnoreActor)
	{
		Query.AddIgnoredActor(IgnoreActor);
	}
	for (const FLamp& Lamp : Lamps)
	{
		if (!Lamp.bOn || !Lamp.Light)
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
	return Level;
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
	int32 Total = 0;
	for (int32 i = 0; i < Walkable.Num(); ++i)
	{
		Total += Walkable[i] ? 1 : 0;
	}
	if (IsWalkable(Blocked))
	{
		--Total;
	}

	TArray<bool> Seen;
	Seen.Init(false, Width * Height);
	TArray<FIntPoint> Queue;
	Queue.Add(PlayerStart);
	Seen[PlayerStart.Y * Width + PlayerStart.X] = true;
	int32 Reached = 0;
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const FIntPoint C = Queue[Head];
		++Reached;
		for (const FIntPoint& D : Dirs)
		{
			const FIntPoint N = C + D;
			if (N == Blocked || !IsWalkable(N) || Seen[N.Y * Width + N.X])
			{
				continue;
			}
			Seen[N.Y * Width + N.X] = true;
			Queue.Add(N);
		}
	}
	return Reached == Total;
}

int32 ATTHospital::SetLampsNear(const FVector& Center, float Radius, bool bOn)
{
	int32 Changed = 0;
	for (FLamp& Lamp : Lamps)
	{
		if (!Lamp.Light || Lamp.bKilled == !bOn)
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
		TTAudio::PlayAt(this, ETTSfx::Click, LampLoc, 0.9f);
		++Changed;
	}
	return Changed;
}

ATTDoor* ATTHospital::GetDoorAt(const FIntPoint& Cell) const
{
	const TObjectPtr<ATTDoor>* Found = Doors.Find(Cell);
	return Found ? Found->Get() : nullptr;
}

FIntPoint ATTHospital::FindNearestWalkable(const FIntPoint& Cell) const
{
	if (IsWalkable(Cell))
	{
		return Cell;
	}
	for (int32 R = 1; R <= 4; ++R)
	{
		for (int32 Y = -R; Y <= R; ++Y)
		{
			for (int32 X = -R; X <= R; ++X)
			{
				if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) == R && IsWalkable(Cell + FIntPoint(X, Y)))
				{
					return Cell + FIntPoint(X, Y);
				}
			}
		}
	}
	return HunterStart;
}

TArray<FIntPoint> ATTHospital::GetWalkableCellsNear(const FIntPoint& Center, int32 Radius) const
{
	TArray<FIntPoint> Result;
	for (int32 Y = -Radius; Y <= Radius; ++Y)
	{
		for (int32 X = -Radius; X <= Radius; ++X)
		{
			const FIntPoint C = Center + FIntPoint(X, Y);
			if (IsWalkable(C) && !Doors.Contains(C))
			{
				Result.Add(C);
			}
		}
	}
	return Result;
}

bool ATTHospital::FindPath(const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPath) const
{
	OutPath.Reset();
	const FIntPoint Start = FindNearestWalkable(From);
	const FIntPoint Goal = FindNearestWalkable(To);
	if (Start == Goal)
	{
		OutPath.Add(Goal);
		return true;
	}

	const int32 Count = Width * Height;
	TArray<float> G;
	G.Init(TNumericLimits<float>::Max(), Count);
	TArray<int32> Parent;
	Parent.Init(INDEX_NONE, Count);
	TArray<bool> Closed;
	Closed.Init(false, Count);
	TArray<int32> Open;

	auto Index = [this](const FIntPoint& P) { return P.Y * Width + P.X; };
	auto Heuristic = [&Goal](const FIntPoint& P) { return static_cast<float>(FMath::Abs(P.X - Goal.X) + FMath::Abs(P.Y - Goal.Y)); };

	G[Index(Start)] = 0.f;
	Open.Add(Index(Start));

	while (Open.Num() > 0)
	{
		int32 BestSlot = 0;
		float BestF = TNumericLimits<float>::Max();
		for (int32 i = 0; i < Open.Num(); ++i)
		{
			const FIntPoint P(Open[i] % Width, Open[i] / Width);
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

		const FIntPoint CP(Current % Width, Current / Width);
		if (CP == Goal)
		{
			for (int32 Node = Current; Node != INDEX_NONE; Node = Parent[Node])
			{
				OutPath.Insert(FIntPoint(Node % Width, Node / Width), 0);
			}
			return true;
		}

		for (const FIntPoint& D : Dirs)
		{
			const FIntPoint NP = CP + D;
			if (!IsWalkable(NP))
			{
				continue;
			}
			const int32 NI = Index(NP);
			const float Cost = G[Current] + 1.f;
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
