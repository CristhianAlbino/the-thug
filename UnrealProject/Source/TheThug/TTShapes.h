#pragma once

#include "CoreMinimal.h"

class AActor;
class ACharacter;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Helpers that build prototype geometry from the engine's BasicShapes.
 * Every shape in /Engine/BasicShapes is 100 units wide, so Size is given in centimetres.
 */
/** Parameters of M_TTSurface (Scripts/create_surface_material.py): world-space tiles, grime, wetness. */
struct FTTSurface
{
	FLinearColor Color = FLinearColor(0.3f, 0.3f, 0.3f);
	FLinearColor Grime = FLinearColor(0.06f, 0.05f, 0.04f);
	float TileSize = 0.f;     // cm, 0 = no tiles
	float Grout = 0.3f;       // cm
	float GrimeAmount = 0.3f; // 0-1
	float Wet = 0.f;          // 0-1 (puddles on floors, sheen on walls)
	float Roughness = 0.8f;
	float Speckle = 0.15f;
	float Seed = 0.f;
	float Metallic = 0.f;
};

namespace TTShapes
{
	UStaticMesh* Cube();
	UStaticMesh* Sphere();
	UStaticMesh* Cylinder();
	UStaticMesh* Cone();

	UMaterialInstanceDynamic* MakeColor(UObject* Outer, const FLinearColor& Color, float Roughness = 0.8f);
	/** Procedural surface (Content/Materials/M_TTSurface). Falls back to a plain colour if the asset is missing. */
	UMaterialInstanceDynamic* MakeSurface(UObject* Outer, const FTTSurface& Surface);
	/** A worn version of a flat colour: what every AddPart gets by default. */
	FTTSurface Worn(const FLinearColor& Color, float Roughness = 0.75f, float Grime = 0.3f);
	FTTSurface Metal(const FLinearColor& Color, float Rust = 0.35f);
	/** Unlit glow (Content/Materials/M_TTEmissive). Falls back to a plain colour if the asset is missing. */
	UMaterialInstanceDynamic* MakeEmissive(UObject* Outer, const FLinearColor& Color, float Intensity);
	/** Lit translucent smoke (Content/Materials/M_TTSmoke). */
	UMaterialInstanceDynamic* MakeSmoke(UObject* Outer, float Opacity);

	/**
	 * Epic's UE5 Mannequin (Content/Characters/Mannequins, from the engine's template resources) with the
	 * ABP_Unarmed locomotion (idle / walk / jog from CharacterMovement). Tints the two material slots
	 * (0 = head and legs, 1 = torso). Scale 1 = 180 cm. Returns null if the assets are missing.
	 */
	USkeletalMeshComponent* SetupMannequin(ACharacter* Character, bool bQuinn, const FLinearColor& HeadLegs, const FLinearColor& Torso, float Scale = 1.f, float Roughness = 0.6f);
	void TintMannequin(ACharacter* Character, int32 Slot, const FLinearColor& Tint);

	UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		const FVector& RelativeLocation, const FVector& Size, const FLinearColor& Color,
		const FRotator& RelativeRotation = FRotator::ZeroRotator, bool bCollision = true);
}
