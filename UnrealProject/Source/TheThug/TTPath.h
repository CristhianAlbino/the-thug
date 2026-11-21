#pragma once

#include "CoreMinimal.h"

class ACharacter;
class ATTDoor;
class ATTHospital;

/**
 * Grid path + string pulling + door opening, shared by the companions.
 * (The Hunter keeps his own copy with his door-memory hooks.)
 */
struct FTTPathFollower
{
	TArray<FVector> Path;
	int32 Index = 0;
	float DoorWait = 0.f;
	float StuckTime = 0.f;

	bool Build(ACharacter* Self, ATTHospital* Hospital, const FVector& Goal, const AActor* Ignore = nullptr);
	/** Moves Self along the path. Returns true when the end is reached (or there is no path). */
	bool Follow(ACharacter* Self, ATTHospital* Hospital, float DeltaSeconds, float Speed);
	void Clear() { Path.Reset(); Index = 0; }
	bool HasPath() const { return Index < Path.Num(); }
};
