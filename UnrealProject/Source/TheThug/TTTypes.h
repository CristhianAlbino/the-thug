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
	Breath,
	GlassCrunch,
	Gunshot,
	Explosion
};

/** Escape-route devices (MapCore actions). */
UENUM()
enum class ETTStation : uint8
{
	ClueA,
	ClueB,
	ClueC,
	GuardLog,
	SecurityNote,
	Safe,
	Generator,
	GateControl,
	Radio,
	Interference,
	Helipad,
	TunnelExit,
	// Narrative (M07)
	Document,
	ThomasBody,
	Explosives,
	Charge,
	Detonator
};

/** Items Joe can carry (ids match the SurvivalCore inventory catalog). */
UENUM()
enum class ETTItem : uint8
{
	Rifle,
	Magazine,
	Grenade,
	Smoke,
	Bottle,
	Bandage,
	Medkit,
	Crowbar,
	Batteries
};

inline const TCHAR* TTItemId(ETTItem Item)
{
	switch (Item)
	{
	case ETTItem::Rifle: return TEXT("rifle");
	case ETTItem::Magazine: return TEXT("magazine");
	case ETTItem::Grenade: return TEXT("grenade");
	case ETTItem::Smoke: return TEXT("smoke");
	case ETTItem::Bottle: return TEXT("bottle");
	case ETTItem::Bandage: return TEXT("bandage");
	case ETTItem::Medkit: return TEXT("medkit");
	case ETTItem::Crowbar: return TEXT("crowbar");
	case ETTItem::Batteries: return TEXT("batteries");
	}
	return TEXT("bottle");
}

inline FString TTItemName(ETTItem Item)
{
	switch (Item)
	{
	case ETTItem::Rifle: return TEXT("Fuzil");
	case ETTItem::Magazine: return TEXT("Carregador");
	case ETTItem::Grenade: return TEXT("Granada");
	case ETTItem::Smoke: return TEXT("Granada de fumaça");
	case ETTItem::Bottle: return TEXT("Garrafa");
	case ETTItem::Bandage: return TEXT("Atadura");
	case ETTItem::Medkit: return TEXT("Kit médico");
	case ETTItem::Crowbar: return TEXT("Pé de cabra");
	case ETTItem::Batteries: return TEXT("Pilhas");
	}
	return TEXT("?");
}

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
