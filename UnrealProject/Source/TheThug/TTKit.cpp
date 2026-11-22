#include "TTKit.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	struct FKitEntry
	{
		const TCHAR* Key;
		const TCHAR* Path;
	};

#include "TTKitAssets.inl"

	template <int32 N>
	int32 CountOf(const FKitEntry (&Table)[N], const TCHAR* Key)
	{
		int32 Count = 0;
		for (const FKitEntry& E : Table)
		{
			Count += FCString::Strcmp(E.Key, Key) == 0 ? 1 : 0;
		}
		return Count;
	}
}

bool TTKit::IsAvailable()
{
	static int32 Cached = -1;
	if (Cached < 0)
	{
		// -ttnoart: compare behaviour with the procedural art only (tests).
		Cached = (!FParse::Param(FCommandLine::Get(), TEXT("ttnoart")) && UE_ARRAY_COUNT(KitMaterials) > 0 && Material(TEXT("wall_paint")) != nullptr) ? 1 : 0;
		UE_LOG(LogTemp, Log, TEXT("[TT] Art kit (Horror Hospital) %s"), Cached ? TEXT("found") : TEXT("missing: procedural art only"));
	}
	return Cached == 1;
}

int32 TTKit::Count(const TCHAR* Category)
{
	return CountOf(KitMeshes, Category);
}

UStaticMesh* TTKit::Mesh(const TCHAR* Category, int32 Pick)
{
	const int32 N = Count(Category);
	if (N == 0 || !IsAvailable())
	{
		return nullptr;
	}
	int32 Want = ((Pick % N) + N) % N;
	for (const FKitEntry& E : KitMeshes)
	{
		if (FCString::Strcmp(E.Key, Category) == 0 && Want-- == 0)
		{
			return LoadObject<UStaticMesh>(nullptr, E.Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}
	return nullptr;
}

UMaterialInterface* TTKit::Material(const TCHAR* Key)
{
	for (const FKitEntry& E : KitMaterials)
	{
		if (FCString::Strcmp(E.Key, Key) == 0)
		{
			return LoadObject<UMaterialInterface>(nullptr, E.Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
	}
	return nullptr;
}

UStaticMeshComponent* TTKit::Place(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, float Yaw,
	bool bCollision, float MaxXY, bool bAlignLong, bool bHang)
{
	if (!Owner || !Mesh || !IsAvailable())
	{
		return nullptr;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector Ext = Bounds.BoxExtent;
	const FVector Org = Bounds.Origin;
	const float YawFix = (bAlignLong && Ext.Y > Ext.X) ? 90.f : 0.f;
	float Scale = 1.f;
	if (MaxXY > 0.f)
	{
		Scale = FMath::Min(1.f, MaxXY / FMath::Max(1.f, 2.f * FMath::Max(Ext.X, Ext.Y)));
	}
	const FRotator Rot(0.f, Yaw + YawFix, 0.f);
	FVector Rel = Location - Rot.RotateVector(FVector(Org.X, Org.Y, 0.f) * Scale);
	Rel.Z = Location.Z - (bHang ? (Org.Z + Ext.Z) : (Org.Z - Ext.Z)) * Scale;

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(Mesh);
	Comp->SetMobility(EComponentMobility::Movable);
	if (Parent)
	{
		Comp->SetupAttachment(Parent);
	}
	Comp->SetRelativeLocation(Rel);
	Comp->SetRelativeRotation(Rot);
	Comp->SetRelativeScale3D(FVector(Scale));
	Comp->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	return Comp;
}
