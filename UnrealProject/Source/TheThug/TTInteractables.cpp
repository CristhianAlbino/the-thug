#include "TTInteractables.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TTAudio.h"
#include "TTGameMode.h"
#include "TTPlayerCharacter.h"
#include "TTShapes.h"

namespace
{
	ATTGameMode* GetTTGameMode(const UObject* Context)
	{
		const UWorld* World = Context ? Context->GetWorld() : nullptr;
		return World ? World->GetAuthGameMode<ATTGameMode>() : nullptr;
	}

	const FLinearColor DoorColor(0.16f, 0.12f, 0.09f);
	const FLinearColor FrameColor(0.22f, 0.24f, 0.22f);
	const FLinearColor LockerColor(0.20f, 0.24f, 0.26f);
	const FLinearColor LockerDoorColor(0.17f, 0.21f, 0.23f);
}

// ---------------------------------------------------------------------------
// Interactable

ATTInteractable::ATTInteractable()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	PrimaryActorTick.bCanEverTick = false;
}

// ---------------------------------------------------------------------------
// Door

ATTDoor::ATTDoor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATTDoor::Setup(float CellSize, float WallHeight)
{
	const float DoorHeight = 255.f;
	const float Length = CellSize - 24.f;

	Hinge = NewObject<USceneComponent>(this, TEXT("Hinge"));
	Hinge->SetMobility(EComponentMobility::Movable);
	Hinge->SetupAttachment(Root);
	Hinge->SetRelativeLocation(FVector(-CellSize * 0.5f + 12.f, 0.f, 0.f));
	Hinge->RegisterComponent();

	TTShapes::AddPart(this, Hinge, TTShapes::Cube(), FVector(Length * 0.5f, 0.f, DoorHeight * 0.5f), FVector(Length, 7.f, DoorHeight), DoorColor);
	// Small window and handle so the door reads as a door in the dark.
	TTShapes::AddPart(this, Hinge, TTShapes::Cube(), FVector(Length * 0.5f, 0.f, 185.f), FVector(60.f, 8.f, 45.f), FLinearColor(0.05f, 0.06f, 0.06f), FRotator::ZeroRotator, false);
	TTShapes::AddPart(this, Hinge, TTShapes::Cube(), FVector(Length - 25.f, 0.f, 100.f), FVector(14.f, 14.f, 4.f), FLinearColor(0.5f, 0.5f, 0.48f), FRotator::ZeroRotator, false);

	// Lintel above the door fills the wall cell.
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, DoorHeight + (WallHeight - DoorHeight) * 0.5f), FVector(CellSize, CellSize, WallHeight - DoorHeight), FrameColor);
	// Door frame posts.
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(-CellSize * 0.5f + 6.f, 0.f, DoorHeight * 0.5f), FVector(12.f, CellSize, DoorHeight), FrameColor);
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(CellSize * 0.5f - 6.f, 0.f, DoorHeight * 0.5f), FVector(12.f, CellSize, DoorHeight), FrameColor);
}

void ATTDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Hinge && IsMoving())
	{
		Angle = FMath::FInterpConstantTo(Angle, TargetAngle, DeltaSeconds, Speed);
		Hinge->SetRelativeRotation(FRotator(0.f, Angle, 0.f));
	}
}

FString ATTDoor::GetPrompt(const ATTPlayerCharacter* Player) const
{
	if (bOpen)
	{
		return TEXT("[E] Fechar porta");
	}
	return Player && Player->bIsCrouched ? TEXT("[E] Abrir devagar (silencioso)") : TEXT("[E] Abrir porta   (agachado = silencioso)");
}

void ATTDoor::Interact(ATTPlayerCharacter* Player)
{
	SetOpen(!bOpen, Player && Player->bIsCrouched, Player);
}

void ATTDoor::SetOpen(bool bInOpen, bool bQuiet, const AActor* Opener)
{
	if (bOpen == bInOpen)
	{
		return;
	}
	bOpen = bInOpen;

	if (bOpen)
	{
		float Side = 1.f;
		if (Opener)
		{
			const FVector Local = GetActorTransform().InverseTransformPosition(Opener->GetActorLocation());
			Side = Local.Y > 0.f ? -1.f : 1.f;
		}
		TargetAngle = 95.f * Side;
	}
	else
	{
		TargetAngle = 0.f;
	}

	const bool bByPlayer = Opener && Opener->IsA<ATTPlayerCharacter>();
	Speed = bQuiet ? 60.f : (bByPlayer ? 210.f : 320.f);

	const FVector SoundLoc = GetActorLocation() + FVector(0.f, 0.f, 150.f);
	TTAudio::PlayAt(this, ETTSfx::DoorCreak, SoundLoc, bQuiet ? 0.25f : 0.9f);

	if (bByPlayer)
	{
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->ReportNoise(GetActorLocation(), bQuiet ? 0.1f : 0.42f, const_cast<AActor*>(Opener));
		}
	}
}

// ---------------------------------------------------------------------------
// Locker

namespace LockerDims
{
	constexpr float Depth = 62.f;
	constexpr float Width = 92.f;
	constexpr float Height = 205.f;
}

ATTLocker::ATTLocker()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATTLocker::Setup()
{
	using namespace LockerDims;
	UStaticMesh* Cube = TTShapes::Cube();

	TTShapes::AddPart(this, Root, Cube, FVector(-Depth * 0.5f, 0.f, Height * 0.5f), FVector(3.f, Width, Height), LockerColor);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, -Width * 0.5f, Height * 0.5f), FVector(Depth, 3.f, Height), LockerColor);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, Width * 0.5f, Height * 0.5f), FVector(Depth, 3.f, Height), LockerColor);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, 0.f, Height), FVector(Depth, Width, 3.f), LockerColor);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, 0.f, 2.f), FVector(Depth, Width, 4.f), LockerColor);

	DoorHinge = NewObject<USceneComponent>(this, TEXT("DoorHinge"));
	DoorHinge->SetMobility(EComponentMobility::Movable);
	DoorHinge->SetupAttachment(Root);
	DoorHinge->SetRelativeLocation(FVector(Depth * 0.5f, -Width * 0.5f, 0.f));
	DoorHinge->RegisterComponent();

	const float HalfW = Width * 0.5f;
	TTShapes::AddPart(this, DoorHinge, Cube, FVector(0.f, HalfW, 72.f), FVector(3.f, Width, 144.f), LockerDoorColor);
	const float SlatZ[] = { 146.f, 155.f, 164.f, 173.f };
	for (float Z : SlatZ)
	{
		TTShapes::AddPart(this, DoorHinge, Cube, FVector(0.f, HalfW, Z), FVector(3.f, Width, 4.f), LockerDoorColor);
	}
	TTShapes::AddPart(this, DoorHinge, Cube, FVector(0.f, HalfW, 191.5f), FVector(3.f, Width, 27.f), LockerDoorColor);
	TTShapes::AddPart(this, DoorHinge, Cube, FVector(3.f, Width - 12.f, 110.f), FVector(4.f, 4.f, 18.f), FLinearColor(0.45f, 0.45f, 0.42f), FRotator::ZeroRotator, false);
}

void ATTLocker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DoorTimer = FMath::Max(0.f, DoorTimer - DeltaSeconds);
	const float Target = DoorTimer > 0.f ? 100.f : 0.f;
	if (!FMath::IsNearlyEqual(DoorAngle, Target, 0.5f) && DoorHinge)
	{
		DoorAngle = FMath::FInterpConstantTo(DoorAngle, Target, DeltaSeconds, 420.f);
		DoorHinge->SetRelativeRotation(FRotator(0.f, -DoorAngle, 0.f));
	}
}

FString ATTLocker::GetPrompt(const ATTPlayerCharacter* Player) const
{
	if (Player && Player->GetHiddenLocker() == this)
	{
		return TEXT("[E] Sair do armário");
	}
	return bOccupied ? FString() : TEXT("[E] Esconder-se no armário");
}

void ATTLocker::Interact(ATTPlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}
	if (Player->GetHiddenLocker() == this)
	{
		Player->ExitLocker();
	}
	else if (!bOccupied)
	{
		++Uses;
		Player->EnterLocker(this);
	}
}

FVector ATTLocker::GetInsideLocation() const
{
	return GetActorLocation() - GetActorForwardVector() * 4.f;
}

FVector ATTLocker::GetFrontLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * (LockerDims::Depth * 0.5f + 65.f);
}

float ATTLocker::GetSuspicion() const
{
	static const float Table[] = { 0.f, 10.f, 30.f, 60.f, 90.f };
	return Table[FMath::Clamp(Uses, 0, 4)];
}

void ATTLocker::SetOccupied(bool bInOccupied)
{
	bOccupied = bInOccupied;
	FlapDoor();
}

void ATTLocker::FlapDoor()
{
	DoorTimer = 0.35f;
	TTAudio::PlayAt(this, ETTSfx::Clank, GetActorLocation() + FVector(0.f, 0.f, 120.f), 0.45f);
}

// ---------------------------------------------------------------------------
// Pickup

void ATTPickup::Setup(ETTPickupType InType)
{
	Type = InType;
	if (Type == ETTPickupType::Key)
	{
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 38.f), FVector(90.f, 60.f, 76.f), FLinearColor(0.18f, 0.17f, 0.15f));
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 77.f), FVector(16.f, 10.f, 1.5f), FLinearColor(0.95f, 0.72f, 0.08f), FRotator(0.f, 25.f, 0.f));

		UPointLightComponent* Glow = NewObject<UPointLightComponent>(this);
		Glow->SetupAttachment(Root);
		Glow->SetRelativeLocation(FVector(0.f, 0.f, 95.f));
		Glow->SetIntensityUnits(ELightUnits::Lumens);
		Glow->SetIntensity(6.f);
		Glow->SetAttenuationRadius(140.f);
		Glow->SetLightColor(FLinearColor(1.f, 0.75f, 0.3f));
		Glow->SetCastShadows(false);
		Glow->RegisterComponent();
	}
	else
	{
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 0.f, 13.f), FVector(9.f, 9.f, 26.f), FLinearColor(0.06f, 0.16f, 0.07f), FRotator::ZeroRotator, true);
	}
}

FString ATTPickup::GetPrompt(const ATTPlayerCharacter* Player) const
{
	return Type == ETTPickupType::Key ? TEXT("[E] Pegar cartão de acesso") : TEXT("[E] Pegar garrafa");
}

void ATTPickup::Interact(ATTPlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}
	TTAudio::Play2D(this, ETTSfx::Pickup, 0.6f);
	if (Type == ETTPickupType::Key)
	{
		Player->GiveKey();
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->ShowMessage(TEXT("Cartão do portão de serviço. A saída fica no fim do corredor principal (leste)."), 6.f);
			GM->SetObjective(TEXT("Chegue ao portão de serviço no fim do corredor leste."));
		}
	}
	else
	{
		Player->AddBottles(1);
	}
	Destroy();
}

// ---------------------------------------------------------------------------
// Exit gate

void ATTExitGate::Setup(float CellSize, float WallHeight)
{
	const FLinearColor Metal(0.14f, 0.05f, 0.04f);
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, WallHeight * 0.5f), FVector(30.f, CellSize, WallHeight), Metal);
	for (int32 i = 0; i < 8; ++i)
	{
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(17.f, 0.f, 30.f + i * 36.f), FVector(6.f, CellSize - 20.f, 6.f), FLinearColor(0.1f, 0.04f, 0.03f), FRotator::ZeroRotator, false);
	}
	TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(20.f, CellSize * 0.5f - 45.f, 130.f), FVector(8.f, 22.f, 32.f), FLinearColor(0.05f, 0.05f, 0.05f));

	StatusLight = NewObject<UPointLightComponent>(this);
	StatusLight->SetupAttachment(Root);
	StatusLight->SetRelativeLocation(FVector(35.f, CellSize * 0.5f - 45.f, 140.f));
	StatusLight->SetIntensityUnits(ELightUnits::Lumens);
	StatusLight->SetIntensity(10.f);
	StatusLight->SetAttenuationRadius(160.f);
	StatusLight->SetLightColor(FLinearColor(1.f, 0.05f, 0.02f));
	StatusLight->SetCastShadows(false);
	StatusLight->RegisterComponent();
}

FString ATTExitGate::GetPrompt(const ATTPlayerCharacter* Player) const
{
	return Player && Player->HasKey() ? TEXT("[E] Passar o cartão no portão") : TEXT("[E] Portão de serviço (trancado)");
}

void ATTExitGate::Interact(ATTPlayerCharacter* Player)
{
	ATTGameMode* GM = GetTTGameMode(this);
	if (Player && Player->HasKey())
	{
		SetUnlocked();
		TTAudio::Play2D(this, ETTSfx::Pickup, 0.8f);
		if (GM)
		{
			GM->PlayerEscaped();
		}
	}
	else
	{
		TTAudio::Play2D(this, ETTSfx::Click, 0.6f);
		if (GM)
		{
			GM->ShowMessage(TEXT("Trancado. O leitor pede um cartão de acesso. Deve estar em algum lugar deste andar."), 5.f);
			GM->SetObjective(TEXT("Encontre o cartão de acesso do portão de serviço."));
		}
	}
}

void ATTExitGate::SetUnlocked()
{
	if (StatusLight)
	{
		StatusLight->SetLightColor(FLinearColor(0.1f, 1.f, 0.2f));
	}
}

// ---------------------------------------------------------------------------
// Throwable

ATTThrowable::ATTThrowable()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(6.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 0.f;
	Movement->MaxSpeed = 3000.f;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 1.f;
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 10.f;
}

void ATTThrowable::Launch(const FVector& Velocity, AActor* Thrower)
{
	SetInstigator(Cast<APawn>(Thrower));
	if (Thrower)
	{
		Collision->IgnoreActorWhenMoving(Thrower, true);
	}
	TTShapes::AddPart(this, Collision, TTShapes::Cylinder(), FVector::ZeroVector, FVector(8.f, 8.f, 24.f), FLinearColor(0.06f, 0.16f, 0.07f), FRotator(90.f, 0.f, 0.f), false);
	Movement->OnProjectileStop.AddDynamic(this, &ATTThrowable::OnStop);
	Movement->Velocity = Velocity;
}

void ATTThrowable::OnStop(const FHitResult& ImpactResult)
{
	if (bBroken)
	{
		return;
	}
	bBroken = true;
	const FVector Loc = GetActorLocation();
	TTAudio::PlayAt(this, ETTSfx::Shatter, Loc, 1.f);
	if (ATTGameMode* GM = GetTTGameMode(this))
	{
		GM->ReportNoise(Loc, 1.f, GetInstigator());
	}
	Destroy();
}
