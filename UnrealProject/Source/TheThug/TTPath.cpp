#include "TTPath.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TTHospital.h"
#include "TTInteractables.h"

namespace
{
	bool IsSegmentClear(const UWorld* World, const AActor* Self, const AActor* Ignore, const FVector& From, const FVector& To)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(TTPath), false, Self);
		if (Ignore)
		{
			Query.AddIgnoredActor(Ignore);
		}
		return !World->SweepTestByChannel(From, FVector(To.X, To.Y, From.Z), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(32.f, 55.f), Query);
	}
}

bool FTTPathFollower::Build(ACharacter* Self, ATTHospital* Hospital, const FVector& Goal, const AActor* Ignore)
{
	Path.Reset();
	Index = 0;
	DoorWait = 0.f;
	StuckTime = 0.f;
	if (!Self || !Hospital)
	{
		return false;
	}
	UWorld* World = Self->GetWorld();
	const FVector Start = Self->GetActorLocation();
	const FVector Target(Goal.X, Goal.Y, Start.Z);
	if (IsSegmentClear(World, Self, Ignore, Start, Target))
	{
		Path.Add(Target);
		return true;
	}
	TArray<FIntPoint> Cells;
	if (!Hospital->FindPath(Hospital->WorldToCell(Start), Hospital->WorldToCell(Target), Cells))
	{
		return false;
	}
	TArray<FVector> Raw;
	for (int32 i = 1; i < Cells.Num(); ++i)
	{
		Raw.Add(Hospital->CellToWorld(Cells[i], Start.Z));
	}
	if (Raw.Num() == 0)
	{
		Raw.Add(Hospital->CellToWorld(Cells[0], Start.Z));
	}
	if (Hospital->IsWalkable(Hospital->WorldToCell(Target)))
	{
		Raw.Last() = Target;
	}
	FVector Current = Start;
	int32 i = 0;
	while (i < Raw.Num())
	{
		int32 Best = i;
		for (int32 j = Raw.Num() - 1; j > i; --j)
		{
			if (IsSegmentClear(World, Self, Ignore, Current, Raw[j]))
			{
				Best = j;
				break;
			}
		}
		Path.Add(Raw[Best]);
		Current = Raw[Best];
		i = Best + 1;
	}
	return true;
}

bool FTTPathFollower::Follow(ACharacter* Self, ATTHospital* Hospital, float DeltaSeconds, float Speed)
{
	if (!Self || Index >= Path.Num())
	{
		return true;
	}
	Self->GetCharacterMovement()->MaxWalkSpeed = Speed;
	if (DoorWait > 0.f)
	{
		DoorWait -= DeltaSeconds;
		return false;
	}
	const FVector Loc = Self->GetActorLocation();
	FVector To = Path[Index] - Loc;
	To.Z = 0.f;
	const bool bLast = Index == Path.Num() - 1;
	if (To.Size() < (bLast ? 60.f : 90.f))
	{
		++Index;
		StuckTime = 0.f;
		return Index >= Path.Num();
	}
	const FVector Dir = To.GetSafeNormal();
	if (Hospital)
	{
		const float Segment = To.Size();
		for (const float Ahead : { 0.f, 110.f, 210.f })
		{
			if (Ahead > Segment + 40.f)
			{
				break;
			}
			ATTDoor* Door = Hospital->GetDoorAt(Hospital->WorldToCell(Loc + Dir * Ahead));
			if (Door && !Door->IsOpen() && !Door->IsLocked())
			{
				Door->SetOpen(true, false, Self);
				DoorWait = 0.45f;
				return false;
			}
		}
	}
	Self->AddMovementInput(Dir, 1.f);
	if (Self->GetVelocity().Size2D() < 20.f)
	{
		StuckTime += DeltaSeconds;
		if (StuckTime > 1.2f)
		{
			StuckTime = 0.f;
			++Index;
			return Index >= Path.Num();
		}
	}
	else
	{
		StuckTime = 0.f;
	}
	return false;
}
