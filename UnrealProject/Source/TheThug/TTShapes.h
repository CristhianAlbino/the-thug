#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Helpers that build prototype geometry from the engine's BasicShapes.
 * Every shape in /Engine/BasicShapes is 100 units wide, so Size is given in centimetres.
 */
namespace TTShapes
{
	UStaticMesh* Cube();
	UStaticMesh* Sphere();
	UStaticMesh* Cylinder();
	UStaticMesh* Cone();

	UMaterialInstanceDynamic* MakeColor(UObject* Outer, const FLinearColor& Color, float Roughness = 0.8f);

	UStaticMeshComponent* AddPart(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
		const FVector& RelativeLocation, const FVector& Size, const FLinearColor& Color,
		const FRotator& RelativeRotation = FRotator::ZeroRotator, bool bCollision = true);
}
