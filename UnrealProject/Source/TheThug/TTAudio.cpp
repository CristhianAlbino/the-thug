#include "TTAudio.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	constexpr float TwoPi = 2.f * PI;
	constexpr int32 NumVariants = 4;

	using FBuffer = TArray<int16>;

	void Finish(const TArray<float>& In, FBuffer& Out)
	{
		float Peak = 0.001f;
		for (float S : In)
		{
			Peak = FMath::Max(Peak, FMath::Abs(S));
		}
		const float Gain = Peak > 0.95f ? 0.95f / Peak : 1.f;
		Out.SetNumUninitialized(In.Num());
		for (int32 i = 0; i < In.Num(); ++i)
		{
			Out[i] = static_cast<int16>(FMath::Clamp(In[i] * Gain, -1.f, 1.f) * 32000.f);
		}
	}

	FBuffer Synth(ETTSfx Sfx, int32 Variant)
	{
		FRandomStream Rng(static_cast<int32>(Sfx) * 97 + Variant * 13 + 7);
		const float R = Rng.FRand();
		float Duration = 0.3f;
		switch (Sfx)
		{
		case ETTSfx::HunterStep: Duration = 0.28f; break;
		case ETTSfx::PlayerStep: Duration = 0.14f; break;
		case ETTSfx::Heartbeat: Duration = 0.75f; break;
		case ETTSfx::DoorCreak: Duration = 0.9f + R * 0.3f; break;
		case ETTSfx::Shatter: Duration = 0.8f; break;
		case ETTSfx::Clank: Duration = 0.6f; break;
		case ETTSfx::Sting: Duration = 2.2f; break;
		case ETTSfx::Pickup: Duration = 0.3f; break;
		case ETTSfx::Click: Duration = 0.05f; break;
		}

		const int32 N = static_cast<int32>(Duration * TTAudio::SampleRate);
		TArray<float> S;
		S.SetNumZeroed(N);

		float Lp = 0.f;
		float Lp2 = 0.f;
		double Phase = 0.0;
		TArray<float> Tones;
		for (int32 k = 0; k < 6; ++k)
		{
			Tones.Add(1800.f + Rng.FRand() * 3600.f);
		}

		for (int32 i = 0; i < N; ++i)
		{
			const float t = static_cast<float>(i) / TTAudio::SampleRate;
			const float Noise = Rng.FRandRange(-1.f, 1.f);
			float V = 0.f;

			switch (Sfx)
			{
			case ETTSfx::HunterStep:
			{
				const float F = 52.f + R * 14.f;
				Lp += 0.08f * (Noise - Lp);
				V = FMath::Sin(TwoPi * F * t) * FMath::Exp(-t * 20.f) * 0.9f
					+ Lp * FMath::Exp(-t * 30.f) * 1.6f
					+ Noise * FMath::Exp(-t * 260.f) * 0.25f;
				V *= (1.f - FMath::Exp(-t * 400.f));
				break;
			}
			case ETTSfx::PlayerStep:
			{
				Lp += 0.22f * (Noise - Lp);
				V = Lp * FMath::Exp(-t * 42.f) * 0.9f + FMath::Sin(TwoPi * (85.f + R * 20.f) * t) * FMath::Exp(-t * 45.f) * 0.35f;
				break;
			}
			case ETTSfx::Heartbeat:
			{
				auto Beat = [](float x)
				{
					if (x < 0.f) return 0.f;
					return FMath::Sin(TwoPi * 44.f * x) * FMath::Exp(-x * 13.f) * (1.f - FMath::Exp(-x * 220.f));
				};
				V = Beat(t) + 0.75f * Beat(t - 0.27f);
				break;
			}
			case ETTSfx::DoorCreak:
			{
				const float F = 150.f + R * 40.f + 60.f * FMath::Sin(TwoPi * (2.1f + R) * t) + 22.f * FMath::Sin(TwoPi * 11.f * t);
				Phase += F / TTAudio::SampleRate;
				const float Saw = 2.f * static_cast<float>(Phase - FMath::FloorToDouble(Phase)) - 1.f;
				Lp += 0.12f * (Saw - Lp);
				const float Env = FMath::Pow(FMath::Sin(PI * t / Duration), 0.6f);
				V = (Lp * 0.9f + Noise * 0.05f) * Env;
				break;
			}
			case ETTSfx::Shatter:
			{
				Lp += 0.35f * (Noise - Lp);
				const float Hp = Noise - Lp;
				V = Hp * FMath::Exp(-t * 8.f) * 0.8f + FMath::Sin(TwoPi * 70.f * t) * FMath::Exp(-t * 35.f) * 0.5f;
				for (int32 k = 0; k < Tones.Num(); ++k)
				{
					const float Start = k * 0.025f;
					if (t > Start)
					{
						V += FMath::Sin(TwoPi * Tones[k] * (t - Start)) * FMath::Exp(-(t - Start) * (12.f + k * 4.f)) * 0.18f;
					}
				}
				break;
			}
			case ETTSfx::Clank:
			{
				const float Partials[] = { 310.f, 517.f, 873.f, 1407.f };
				for (int32 k = 0; k < 4; ++k)
				{
					V += FMath::Sin(TwoPi * Partials[k] * (1.f + R * 0.05f) * t) * FMath::Exp(-t * (7.f + k * 5.f)) * (0.45f - k * 0.08f);
				}
				V += Noise * FMath::Exp(-t * 180.f) * 0.5f;
				break;
			}
			case ETTSfx::Sting:
			{
				const float Glide = FMath::Lerp(120.f, 32.f, FMath::Min(1.f, t / 1.6f));
				Phase += Glide / TTAudio::SampleRate;
				Lp += 0.05f * (Noise - Lp);
				Lp2 += 0.4f * (Noise - Lp2);
				const float Env = (1.f - FMath::Exp(-t * 60.f)) * FMath::Exp(-t * 1.3f);
				V = (FMath::Sin(TwoPi * static_cast<float>(Phase)) * 0.7f
					+ Lp * 2.f * 0.5f
					+ (FMath::Sin(TwoPi * 466.f * t) + FMath::Sin(TwoPi * 494.f * t) + FMath::Sin(TwoPi * 233.f * t)) * 0.12f
					+ Lp2 * FMath::Exp(-t * 6.f) * 0.6f) * Env;
				break;
			}
			case ETTSfx::Pickup:
			{
				const float F = t < 0.12f ? 880.f : 1320.f;
				const float Local = t < 0.12f ? t : t - 0.12f;
				V = FMath::Sin(TwoPi * F * t) * FMath::Exp(-Local * 18.f) * 0.4f;
				break;
			}
			case ETTSfx::Click:
			{
				V = Noise * FMath::Exp(-t * 300.f) * 0.6f + FMath::Sin(TwoPi * 2400.f * t) * FMath::Exp(-t * 200.f) * 0.3f;
				break;
			}
			}
			S[i] = V;
		}

		FBuffer Out;
		Finish(S, Out);
		return Out;
	}

	const FBuffer& GetBuffer(ETTSfx Sfx)
	{
		static TMap<uint32, FBuffer> Cache;
		static int32 Counter = 0;
		const int32 Variant = (Counter++) % NumVariants;
		const uint32 Key = static_cast<uint32>(Sfx) * 16u + static_cast<uint32>(Variant);
		if (FBuffer* Found = Cache.Find(Key))
		{
			return *Found;
		}
		return Cache.Add(Key, Synth(Sfx, Variant));
	}

	UAudioComponent* PlayBuffer(const UObject* WorldContext, const FBuffer& Buffer, const FVector* Location, float Volume)
	{
		if (!WorldContext || Buffer.Num() == 0)
		{
			return nullptr;
		}
		UWorld* World = WorldContext->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		USoundWaveProcedural* Wave = TTAudio::MakeStreamingWave(GetTransientPackage());
		Wave->QueueAudio(reinterpret_cast<const uint8*>(Buffer.GetData()), Buffer.Num() * sizeof(int16));

		UAudioComponent* Comp = Location
			? UGameplayStatics::SpawnSoundAtLocation(WorldContext, Wave, *Location, FRotator::ZeroRotator, Volume, 1.f, 0.f, TTAudio::GetAttenuation())
			: UGameplayStatics::SpawnSound2D(WorldContext, Wave, Volume);

		if (Comp)
		{
			const float Length = static_cast<float>(Buffer.Num()) / TTAudio::SampleRate + 0.15f;
			TWeakObjectPtr<UAudioComponent> Weak(Comp);
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Weak]()
			{
				if (Weak.IsValid())
				{
					Weak->Stop();
				}
			}), Length, false);
		}
		return Comp;
	}
}

USoundWaveProcedural* TTAudio::MakeStreamingWave(UObject* Outer)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	return Wave;
}

USoundAttenuation* TTAudio::GetAttenuation()
{
	static TStrongObjectPtr<USoundAttenuation> Attenuation;
	if (!Attenuation.IsValid())
	{
		USoundAttenuation* Att = NewObject<USoundAttenuation>(GetTransientPackage());
		FSoundAttenuationSettings& S = Att->Attenuation;
		S.bAttenuate = true;
		S.bSpatialize = true;
		S.AttenuationShape = EAttenuationShape::Sphere;
		S.AttenuationShapeExtents = FVector(250.f, 0.f, 0.f);
		S.FalloffDistance = 3200.f;
		S.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		S.bEnableOcclusion = true;
		S.OcclusionLowPassFilterFrequency = 900.f;
		S.OcclusionVolumeAttenuation = 0.45f;
		S.OcclusionInterpolationTime = 0.15f;
		Attenuation.Reset(Att);
	}
	return Attenuation.Get();
}

void TTAudio::Play2D(const UObject* WorldContext, ETTSfx Sfx, float Volume)
{
	PlayBuffer(WorldContext, GetBuffer(Sfx), nullptr, Volume);
}

void TTAudio::PlayAt(const UObject* WorldContext, ETTSfx Sfx, const FVector& Location, float Volume)
{
	PlayBuffer(WorldContext, GetBuffer(Sfx), &Location, Volume);
}

void FTTDroneGenerator::Generate(TArray<int16>& Out, int32 NumSamples)
{
	Out.SetNumUninitialized(NumSamples);
	const double Dt = 1.0 / TTAudio::SampleRate;
	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float t = static_cast<float>(T);
		const float Noise = Rng.FRandRange(-1.f, 1.f);
		Lp += 0.02f * (Noise - Lp);
		Lp2 += 0.002f * (Lp - Lp2);

		const float Swell = 0.6f + 0.4f * FMath::Sin(2.f * PI * 0.05f * t);
		const float Beat = 0.5f + 0.5f * FMath::Sin(2.f * PI * 0.13f * t + 1.3f);
		float V = 0.16f * FMath::Sin(2.f * PI * 41.2f * t) * Swell
			+ 0.10f * FMath::Sin(2.f * PI * 55.0f * t)
			+ 0.05f * FMath::Sin(2.f * PI * 61.7f * t) * Beat
			+ 0.03f * FMath::Sin(2.f * PI * 123.5f * t) * (1.f - Beat)
			+ 0.55f * Lp
			+ 1.2f * Lp2;

		Out[i] = static_cast<int16>(FMath::Clamp(V, -1.f, 1.f) * 20000.f);
		T += Dt;
	}
}
