// Compiles the narrative gates (C++ port of the JSON contract) from /Tools into the game module.
// Source of truth: Tools/NarrativeCore/*.json -> emit_cpp.py -> NarrativeData.inl (tested outside Unreal).
#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "../../../../Tools/NarrativeCore/NarrativeCore.cpp"
THIRD_PARTY_INCLUDES_END
