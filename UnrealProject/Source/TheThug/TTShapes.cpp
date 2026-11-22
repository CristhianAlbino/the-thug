#include "TTShapes.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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

UMaterialInstanceDynamic* TTShapes::MakeSurface(UObject* Outer, const FTTSurface& S)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_TTSurface.M_TTSurface"));
	if (!Base)
	{
		return MakeColor(Outer, S.Color, S.Roughness);
	}
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Outer);
	Mid->SetVectorParameterValue(TEXT("Color"), S.Color);
	Mid->SetVectorParameterValue(TEXT("GrimeColor"), S.Grime);
	Mid->SetVectorParameterValue(TEXT("Shape"), FLinearColor(S.TileSize, S.Grout, S.GrimeAmount, S.Wet));
	Mid->SetVectorParameterValue(TEXT("Finish"), FLinearColor(S.Roughness, S.Speckle, S.Seed, S.Metallic));
	return Mid;
}

FTTSurface TTShapes::Worn(const FLinearColor& Color, float Roughness, float Grime)
{
	FTTSurface S;
	S.Color = Color;
	S.Grime = FLinearColor(Color.R * 0.45f, Color.G * 0.4f, Color.B * 0.32f);
	S.GrimeAmount = Grime;
	S.Roughness = Roughness;
	S.Speckle = 0.12f;
	return S;
}

FTTSurface TTShapes::Metal(const FLinearColor& Color, float Rust)
{
	FTTSurface S;
	S.Color = Color;
	S.Grime = FLinearColor(0.22f, 0.09f, 0.03f);
	S.GrimeAmount = Rust;
	S.Roughness = 0.42f;
	S.Speckle = 0.05f;
	S.Metallic = 0.65f;
	return S;
}

USkeletalMeshComponent* TTShapes::SetupMannequin(ACharacter* Character, bool bQuinn, const FLinearColor& HeadLegs, const FLinearColor& Torso, float Scale, float Roughness)
{
	if (!Character || !Character->GetMesh() || FParse::Param(FCommandLine::Get(), TEXT("ttnoart")))
	{
		return nullptr;
	}
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, bQuinn
		? TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
		: TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	UClass* Anim = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	if (!Mesh)
	{
		return nullptr;
	}
	USkeletalMeshComponent* Comp = Character->GetMesh();
	Comp->SetSkeletalMeshAsset(Mesh);
	if (Anim)
	{
		Comp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Comp->SetAnimInstanceClass(Anim);
	}
	const float Half = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	Comp->SetRelativeLocation(FVector(0.f, 0.f, -Half));
	Comp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Comp->SetRelativeScale3D(FVector(Scale));
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(true);
	Comp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		if (UMaterialInstanceDynamic* Mid = Comp->CreateDynamicMaterialInstance(Slot))
		{
			Mid->SetVectorParameterValue(TEXT("Paint Tint"), Slot == 0 ? HeadLegs : Torso);
			Mid->SetScalarParameterValue(TEXT("MetalPaintMetallic"), 0.f);
			Mid->SetScalarParameterValue(TEXT("MetalPaintRoughness"), Roughness);
			Mid->SetVectorParameterValue(TEXT("LogoTint"), Slot == 0 ? HeadLegs : Torso);
		}
	}
	return Comp;
}

void TTShapes::TintMannequin(ACharacter* Character, int32 Slot, const FLinearColor& Tint)
{
	if (Character && Character->GetMesh())
	{
		if (UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Character->GetMesh()->GetMaterial(Slot)))
		{
			Mid->SetVectorParameterValue(TEXT("Paint Tint"), Tint);
			Mid->SetVectorParameterValue(TEXT("LogoTint"), Tint);
		}
	}
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
	Comp->SetMaterial(0, MakeSurface(Comp, Worn(Color)));
	if (!bCollision)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Comp->RegisterComponent();
	Owner->AddInstanceComponent(Comp);
	return Comp;
}
