#pragma once

// Plain (non-UObject) header: the companion's decision core from /Tools/CompanionCore.
// Included only by .cpp files that need the core itself.

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "../../../Tools/CompanionCore/CompanionCore.h"
THIRD_PARTY_INCLUDES_END

struct FTTCompanionMind
{
	thug::companion::CompanionCore Core;

	FTTCompanionMind(thug::companion::Character Who, thug::companion::Difficulty Difficulty, const thug::companion::Traits& Traits, uint64 Seed)
		: Core(Who, Difficulty, Traits, Seed)
	{
	}
};
