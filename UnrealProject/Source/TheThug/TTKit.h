#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Art kit from the Fab pack "Horror Hospital" (Higher Fusion Studio), chosen by Scripts/build_kit.py
 * (TTKitAssets.inl). The pack is not in the repository; when it is missing every call returns null
 * and the hospital keeps its procedural look. Gameplay never depends on it: collisions, navigation
 * and the Hunter's sight stay on the code-built geometry.
 */
namespace TTKit
{
	bool IsAvailable();
	int32 Count(const TCHAR* Category);
	/** Variant Pick (wrapped) of a category, or null. */
	UStaticMesh* Mesh(const TCHAR* Category, int32 Pick);
	UMaterialInterface* Material(const TCHAR* Key);

	/**
	 * Places a kit mesh by its bounds: centred on Location in XY, bottom (or top, bHang) at Location.Z,
	 * longest side along Yaw when bAlignLong. Shrinks (never grows) to fit MaxXY when > 0.
	 */
	UStaticMeshComponent* Place(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, float Yaw,
		bool bCollision, float MaxXY = 0.f, bool bAlignLong = true, bool bHang = false);
}
