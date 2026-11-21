#pragma once

#include "CoreMinimal.h"

class USoundAttenuation;
class USoundWaveProcedural;

/** Every sound in the M01 prototype is synthesised at runtime, so no audio assets are required. */
enum class ETTSfx : uint8
{
	HunterStep,
	PlayerStep,
	Heartbeat,
	DoorCreak,
	Shatter,
	Clank,
	Sting,
	Pickup,
	Click,
	Gunshot,
	Explosion,
	Breath,
	Punch,
	Hiss
};

namespace TTAudio
{
	constexpr int32 SampleRate = 44100;

	void Play2D(const UObject* WorldContext, ETTSfx Sfx, float Volume = 1.f);
	void PlayAt(const UObject* WorldContext, ETTSfx Sfx, const FVector& Location, float Volume = 1.f);

	USoundWaveProcedural* MakeStreamingWave(UObject* Outer);
	USoundAttenuation* GetAttenuation();
}

/** Low hospital drone (ventilation, distant machinery) streamed in small chunks. */
struct FTTDroneGenerator
{
	void Generate(TArray<int16>& Out, int32 NumSamples);

private:
	double T = 0.0;
	float Lp = 0.f;
	float Lp2 = 0.f;
	FRandomStream Rng{ 1337 };
};
