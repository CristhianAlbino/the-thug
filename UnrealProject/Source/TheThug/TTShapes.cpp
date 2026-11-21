#include "TTShapes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	UStaticMesh* LoadShape(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}
}

UStaticMesh* TTShapes::Cube() { return LoadShape(TEXT("/Engine/BasicShapes/Cube.Cube")); }
UStaticMesh* TTShapes::Sphere() { return LoadShape(TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
UStaticMesh* TTShapes::Cylinder() { return LoadShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
UStaticMesh* TTShapes::Cone() { return LoadShape(TEXT("/Engine/BasicShapes/Cone.Cone")); }

UMaterialInstanceDynamic* TTShapes::MakeColor(UObject* Outer, const FLinearColor& Color, float Roughness)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	Mid->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	return Mid;
}

UMaterialInstanceDynamic* TTShapes::MakeEmissive(UObject* Outer, const FLinearColor& Color, float Intensity)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_TTEmissive.M_TTEmissive"));
	if (!Base)
	{
		return MakeColor(Outer, Color);
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	Mid->SetScalarParameterValue(TEXT("Intensity"), Intensity);
	return Mid;
}

UMaterialInstanceDynamic* TTShapes::MakeSmoke(UObject* Outer, float Opacity)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_TTSmoke.M_TTSmoke"));
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	Mid->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	// Self-lit enough to read as a grey wall in a dark corridor; the flashlight adds on top.
	Mid->SetScalarParameterValue(TEXT("Glow"), 0.08f);
	return Mid;
}

UStaticMeshComponent* TTShapes::AddPart(AActor* Owner, USceneComponent* Parent, UStaticMesh* Mesh,
	const FVector& RelativeLocation, const FVector& Size, const FLinearColor& Color,
	const FRotator& RelativeRotation, bool bCollision)
{
	if (!Owner || !Mesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(Mesh);
	Comp->SetMobility(EComponentMobility::Movable);
	if (Parent)
	{
		Comp->SetupAttachment(Parent);
	}
	Comp->SetRelativeLocation(RelativeLocation);
	Comp->SetRelativeRotation(RelativeRotation);
	Comp->SetRelativeScale3D(Size / 100.f);
	Comp->SetMaterial(0, MakeColor(Comp, Color));
	if (!bCollision)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	return Comp;
}
