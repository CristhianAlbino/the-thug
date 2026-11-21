#pragma once

#include "CoreMinimal.h"
#include "TTTypes.generated.h"

UENUM()
enum class ETTDifficulty : uint8
{
	Easy,
	Normal,
	Hard,
	Punitive
};

/** What made a sound. The Hunter only ever receives an approximate position for it. */
UENUM()
enum class ETTNoise : uint8
{
	SlowStep,
	Step,
	SprintStep,
	Door,
	QuietDoor,
	Locker,
	Glass,
	Breath
};

inline FString TTDifficultyName(ETTDifficulty D)
{
	switch (D)
	{
	case ETTDifficulty::Easy: return TEXT("Fácil");
	case ETTDifficulty::Normal: return TEXT("Normal");
	case ETTDifficulty::Hard: return TEXT("Difícil");
	case ETTDifficulty::Punitive: return TEXT("Punitivo");
	}
	return TEXT("Normal");
}
