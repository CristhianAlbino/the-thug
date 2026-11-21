#include "TTInteractables.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TTAudio.h"
#include "TTGameMode.h"
#include "TTHospital.h"
#include "TTHunter.h"
#include "TTCompanion.h"
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

void ATTDoor::Setup(float CellSize, float WallHeight, const FString& InId)
{
	Id = InId;
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
	if (bLocked)
	{
		return TEXT("[E] Porta travada");
	}
	return Player && Player->bIsCrouched ? TEXT("[E] Abrir devagar (silencioso)") : TEXT("[E] Abrir porta   (agachado = silencioso)");
}

void ATTDoor::Interact(ATTPlayerCharacter* Player)
{
	if (bLocked && !bOpen)
	{
		TTAudio::PlayAt(this, ETTSfx::Click, GetActorLocation() + FVector(0.f, 0.f, 100.f), 0.7f);
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->ShowMessage(TEXT("Travada pelo outro lado. Não estava assim antes. Ele bloqueou essa passagem."), 4.f);
			GM->ReportNoise(GetActorLocation(), ETTNoise::QuietDoor, Player);
		}
		return;
	}
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

	if (Opener && !Opener->IsA<ATTHunter>())
	{
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->ReportNoise(GetActorLocation(), bQuiet ? ETTNoise::QuietDoor : ETTNoise::Door, const_cast<AActor*>(Opener));
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

void ATTLocker::Setup(const FString& InId)
{
	Id = InId;
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
	if (bOccupied)
	{
		const ATTCompanion* Companion = Cast<ATTCompanion>(Occupant.Get());
		return Companion ? FString::Printf(TEXT("Ocupado — %s está aí dentro"), *Companion->GetDisplayName()) : FString();
	}
	return TEXT("[E] Esconder-se no armário");
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
	else if (const ATTCompanion* Companion = Cast<ATTCompanion>(Occupant.Get()))
	{
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->ShowSubtitle(FString::Printf(TEXT("%s: Sai daqui! Ele vai ouvir!"), *Companion->GetDisplayName()), 3.f);
		}
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

void ATTLocker::SetOccupied(bool bInOccupied, AActor* InOccupant)
{
	bOccupied = bInOccupied;
	Occupant = bInOccupied ? InOccupant : nullptr;
	FlapDoor();
}

void ATTLocker::FlapDoor()
{
	DoorTimer = 0.35f;
	TTAudio::PlayAt(this, ETTSfx::Clank, GetActorLocation() + FVector(0.f, 0.f, 120.f), 0.45f);
}

// ---------------------------------------------------------------------------
// Pickup

void ATTPickup::Setup(ETTPickupType InType, ETTItem InItem, bool bOnTable)
{
	Type = InType;
	Item = InItem;
	const UStaticMesh* None = nullptr;
	(void)None;

	const bool bTable = Type == ETTPickupType::Key || bOnTable;
	const float Z = bTable ? 77.f : 0.f;
	if (bTable)
	{
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 38.f), FVector(90.f, 60.f, 76.f), FLinearColor(0.18f, 0.17f, 0.15f));
	}

	if (Type == ETTPickupType::Key)
	{
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
		return;
	}

	switch (Item)
	{
	case ETTItem::Bottle:
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 0.f, Z + 13.f), FVector(9.f, 9.f, 26.f), FLinearColor(0.06f, 0.16f, 0.07f));
		break;
	case ETTItem::Bandage:
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 0.f, Z + 5.f), FVector(11.f, 11.f, 9.f), FLinearColor(0.85f, 0.84f, 0.8f));
		break;
	case ETTItem::Medkit:
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, Z + 7.f), FVector(34.f, 22.f, 13.f), FLinearColor(0.8f, 0.8f, 0.78f));
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, Z + 14.f), FVector(16.f, 5.f, 1.f), FLinearColor(0.7f, 0.03f, 0.03f), FRotator::ZeroRotator, false);
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, Z + 14.f), FVector(5.f, 16.f, 1.f), FLinearColor(0.7f, 0.03f, 0.03f), FRotator::ZeroRotator, false);
		break;
	case ETTItem::Magazine:
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, Z + 2.f), FVector(20.f, 7.f, 4.f), FLinearColor(0.05f, 0.05f, 0.05f), FRotator(0.f, 30.f, 0.f));
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(10.f, 12.f, Z + 2.f), FVector(20.f, 7.f, 4.f), FLinearColor(0.05f, 0.05f, 0.05f), FRotator(0.f, -10.f, 0.f));
		break;
	case ETTItem::Smoke:
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 0.f, Z + 8.f), FVector(8.f, 8.f, 16.f), FLinearColor(0.25f, 0.28f, 0.25f));
		break;
	case ETTItem::Grenade:
		TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(0.f, 0.f, Z + 5.f), FVector(9.f, 9.f, 11.f), FLinearColor(0.12f, 0.14f, 0.07f));
		break;
	case ETTItem::Rifle:
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, Z + 4.f), FVector(85.f, 6.f, 9.f), FLinearColor(0.06f, 0.06f, 0.06f));
		break;
	case ETTItem::Crowbar:
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, Z + 2.f), FVector(70.f, 3.f, 3.f), FLinearColor(0.35f, 0.04f, 0.03f), FRotator(0.f, 20.f, 0.f));
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(33.f, 12.f, Z + 2.f), FVector(10.f, 3.f, 3.f), FLinearColor(0.35f, 0.04f, 0.03f), FRotator(0.f, 70.f, 0.f), false);
		break;
	}
}

FString ATTPickup::GetPrompt(const ATTPlayerCharacter* Player) const
{
	if (Type == ETTPickupType::Key)
	{
		return TEXT("[E] Pegar cartão de acesso");
	}
	if (Player && !Player->CanCarry(Item))
	{
		return FString::Printf(TEXT("%s  (sem espaço — [Tab] inventário)"), *TTItemName(Item));
	}
	return FString::Printf(TEXT("[E] Pegar %s"), *TTItemName(Item).ToLower());
}

void ATTPickup::Interact(ATTPlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}
	ATTGameMode* GM = GetTTGameMode(this);
	if (Type == ETTPickupType::Key)
	{
		TTAudio::Play2D(this, ETTSfx::Pickup, 0.6f);
		Player->GiveKey();
		if (GM)
		{
			GM->ShowMessage(TEXT("Cartão do portão de serviço. A saída fica no fim do corredor principal (leste)."), 6.f);
			GM->SetObjective(TEXT("Chegue ao portão de serviço no fim do corredor leste."));
		}
		Destroy();
		return;
	}
	if (!Player->TryAddItem(Item, 1))
	{
		if (GM)
		{
			GM->ShowMessage(FString::Printf(TEXT("Sem espaço para %s. Segure [Tab] e largue algo."), *TTItemName(Item).ToLower()), 3.f);
		}
		return;
	}
	TTAudio::Play2D(this, ETTSfx::Pickup, 0.45f);
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
// Window

void ATTWindow::Setup(float CellSize, float WallHeight, const FVector& InApproach)
{
	Approach = InApproach;
	const FLinearColor Wall(0.34f, 0.37f, 0.33f);
	const float Open = 170.f;
	const float Side = (CellSize - Open) * 0.5f;
	const float Bottom = 95.f;
	const float Top = 235.f;
	UStaticMesh* Cube = TTShapes::Cube();
	// Wall around the opening (the actor faces into the room along +X).
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, -(Open + Side) * 0.5f, WallHeight * 0.5f), FVector(CellSize, Side, WallHeight), Wall);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, (Open + Side) * 0.5f, WallHeight * 0.5f), FVector(CellSize, Side, WallHeight), Wall);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, 0.f, Bottom * 0.5f), FVector(CellSize, Open, Bottom), Wall);
	TTShapes::AddPart(this, Root, Cube, FVector(0.f, 0.f, Top + (WallHeight - Top) * 0.5f), FVector(CellSize, Open, WallHeight - Top), Wall);
	// Glass pane far side, and the boards nailed on the inside.
	TTShapes::AddPart(this, Root, Cube, FVector(-CellSize * 0.5f + 10.f, 0.f, (Bottom + Top) * 0.5f), FVector(4.f, Open, Top - Bottom), FLinearColor(0.02f, 0.03f, 0.05f));
	const float BoardZ[] = { Bottom + 22.f, (Bottom + Top) * 0.5f, Top - 22.f };
	for (int32 i = 0; i < 3; ++i)
	{
		Boards.Add(TTShapes::AddPart(this, Root, Cube, FVector(CellSize * 0.5f + 4.f, 0.f, BoardZ[i]), FVector(5.f, Open + 50.f, 24.f),
			FLinearColor(0.22f, 0.15f, 0.09f), FRotator(0.f, 0.f, i == 1 ? 8.f : -5.f)));
	}
	// Cold light from the street behind the glass.
	UPointLightComponent* Moon = NewObject<UPointLightComponent>(this);
	Moon->SetupAttachment(Root);
	Moon->SetRelativeLocation(FVector(-CellSize * 0.5f - 60.f, 0.f, 200.f));
	Moon->SetIntensityUnits(ELightUnits::Lumens);
	Moon->SetIntensity(120.f);
	Moon->SetAttenuationRadius(700.f);
	Moon->SetLightColor(FLinearColor(0.5f, 0.6f, 0.9f));
	Moon->RegisterComponent();
}

FString ATTWindow::GetPrompt(const ATTPlayerCharacter* Player) const
{
	if (bOpen)
	{
		return TEXT("[E] Fugir pela janela para o prédio vizinho");
	}
	if (Player && Player->GetItemCount(ETTItem::Crowbar) > 0)
	{
		return TEXT("[E] Arrancar as tábuas com o pé de cabra (barulhento)");
	}
	return TEXT("Janela pregada — precisa de uma ferramenta");
}

void ATTWindow::Interact(ATTPlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}
	if (bOpen)
	{
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->PlayerEscaped(TEXT("Pela janela, para o prédio vizinho."));
		}
		return;
	}
	if (Player->GetItemCount(ETTItem::Crowbar) > 0)
	{
		Player->StartPrying(this);
	}
	else if (ATTGameMode* GM = GetTTGameMode(this))
	{
		GM->ShowMessage(TEXT("As tábuas estão pregadas por dentro. Com um pé de cabra daria."), 3.f);
	}
}

bool ATTWindow::AddPry(float DeltaSeconds, AActor* By)
{
	if (bOpen)
	{
		return true;
	}
	Progress += DeltaSeconds;
	NoiseTimer -= DeltaSeconds;
	if (NoiseTimer <= 0.f)
	{
		NoiseTimer = 1.5f;
		TTAudio::PlayAt(this, ETTSfx::DoorCreak, GetActorLocation() + FVector(0.f, 0.f, 150.f), 1.f);
		if (ATTGameMode* GM = GetTTGameMode(this))
		{
			GM->ReportNoise(GetActorLocation(), ETTNoise::Door, By);
		}
	}
	const int32 Removed = FMath::Clamp(FMath::FloorToInt(Progress / PryTime * Boards.Num()), 0, Boards.Num());
	for (int32 i = 0; i < Removed; ++i)
	{
		if (Boards[i])
		{
			Boards[i]->SetVisibility(false);
			Boards[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	if (Progress >= PryTime)
	{
		bOpen = true;
		TTAudio::PlayAt(this, ETTSfx::Clank, GetActorLocation() + FVector(0.f, 0.f, 150.f), 1.f);
		UE_LOG(LogTemp, Log, TEXT("[TT] Window open (by %s)"), By ? *By->GetName() : TEXT("?"));
	}
	return bOpen;
}

// ---------------------------------------------------------------------------
// Throwable

ATTThrowable::ATTThrowable()
{
	PrimaryActorTick.bCanEverTick = true;

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

	InitialLifeSpan = 30.f;
}

void ATTThrowable::Launch(const FVector& Velocity, AActor* Thrower, ETTThrowableType InType)
{
	Type = InType;
	SetInstigator(Cast<APawn>(Thrower));
	if (Thrower)
	{
		Collision->IgnoreActorWhenMoving(Thrower, true);
	}

	switch (Type)
	{
	case ETTThrowableType::Bottle:
		TTShapes::AddPart(this, Collision, TTShapes::Cylinder(), FVector::ZeroVector, FVector(8.f, 8.f, 24.f), FLinearColor(0.06f, 0.16f, 0.07f), FRotator(90.f, 0.f, 0.f), false);
		break;
	case ETTThrowableType::Smoke:
		TTShapes::AddPart(this, Collision, TTShapes::Cylinder(), FVector::ZeroVector, FVector(8.f, 8.f, 16.f), FLinearColor(0.25f, 0.28f, 0.25f), FRotator(90.f, 0.f, 0.f), false);
		Movement->bShouldBounce = true;
		Movement->Bounciness = 0.25f;
		Movement->Friction = 0.6f;
		break;
	case ETTThrowableType::Grenade:
		TTShapes::AddPart(this, Collision, TTShapes::Sphere(), FVector::ZeroVector, FVector(9.f, 9.f, 11.f), FLinearColor(0.12f, 0.14f, 0.07f), FRotator::ZeroRotator, false);
		Movement->bShouldBounce = true;
		Movement->Bounciness = 0.35f;
		Movement->Friction = 0.5f;
		break;
	}
	Movement->OnProjectileStop.AddDynamic(this, &ATTThrowable::OnStop);
	Movement->Velocity = Velocity;
}

void ATTThrowable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone)
	{
		return;
	}
	Age += DeltaSeconds;
	if (bStopped)
	{
		StoppedTime += DeltaSeconds;
	}
	if (Type == ETTThrowableType::Grenade && Age >= 2.2f)
	{
		Explode();
	}
	else if (Type == ETTThrowableType::Smoke && (StoppedTime >= 0.4f || Age >= 4.f))
	{
		ReleaseSmoke();
	}
}

void ATTThrowable::OnStop(const FHitResult& ImpactResult)
{
	bStopped = true;
	if (Type != ETTThrowableType::Bottle || bDone)
	{
		return;
	}
	bDone = true;
	const FVector Loc = GetActorLocation();
	TTAudio::PlayAt(this, ETTSfx::Shatter, Loc, 1.f);
	if (ATTGameMode* GM = GetTTGameMode(this))
	{
		GM->ReportNoise(Loc, ETTNoise::Glass, GetInstigator());
	}
	Destroy();
}

void ATTThrowable::ReleaseSmoke()
{
	bDone = true;
	const FVector Loc = GetActorLocation();
	TTAudio::PlayAt(this, ETTSfx::Hiss, Loc, 0.8f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<ATTSmokeCloud>(ATTSmokeCloud::StaticClass(), Loc + FVector(0.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
	Destroy();
}

void ATTThrowable::Explode()
{
	bDone = true;
	const FVector Loc = GetActorLocation() + FVector(0.f, 0.f, 20.f);
	UWorld* World = GetWorld();
	ATTGameMode* GM = GetTTGameMode(this);

	TTAudio::PlayAt(this, ETTSfx::Explosion, Loc, 1.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATTFlash* Flash = World->SpawnActor<ATTFlash>(ATTFlash::StaticClass(), Loc + FVector(0.f, 0.f, 40.f), FRotator::ZeroRotator, Params))
	{
		Flash->Setup(60000.f, 1600.f, 0.45f, FLinearColor(1.f, 0.7f, 0.4f));
	}

	auto HasLineTo = [World, this, &Loc](const AActor* Target, const FVector& Point)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(TTExplosion), false, this);
		Query.AddIgnoredActor(Target);
		return !World->LineTraceTestByChannel(Loc, Point, ECC_Visibility, Query);
	};

	if (GM)
	{
		GM->ReportNoise(Loc, ETTNoise::Explosion, GetInstigator());
		if (ATTHospital* Hospital = GM->GetHospital())
		{
			Hospital->SetLampsNear(Loc, 550.f, false);
			// Blows nearby doors open, including one he blocked (GDD §38: "abrir caminhos").
			for (const TPair<FIntPoint, TObjectPtr<ATTDoor>>& Pair : Hospital->GetDoors())
			{
				ATTDoor* Door = Pair.Value;
				if (Door && FVector::Dist(Door->GetActorLocation(), Loc) < 380.f)
				{
					Door->SetLocked(false);
					Door->SetOpen(true, false, this);
				}
			}
		}
		if (ATTHunter* Hunter = GM->GetHunter())
		{
			const float D = FVector::Dist(Hunter->GetActorLocation(), Loc);
			if (D < 550.f && HasLineTo(Hunter, Hunter->GetActorLocation()))
			{
				Hunter->Stun(D < 300.f ? 3.5f : 2.2f, true);
			}
		}
		if (ATTPlayerCharacter* Player = GM->GetPlayer())
		{
			const float D = FVector::Dist(Player->GetActorLocation(), Loc);
			if (D < 650.f && HasLineTo(Player, Player->GetActorLocation()))
			{
				Player->TakeInjury(D < 330.f ? 3 : 2, TEXT("A explosão te pegou."));
			}
		}
	}
	Destroy();
}

// ---------------------------------------------------------------------------
// Smoke cloud

namespace SmokeTuning
{
	constexpr float MaxRadius = 380.f;
	constexpr float GrowTime = 2.f;
	constexpr float HoldTime = 14.f;
	constexpr float FadeTime = 3.f;
}

ATTSmokeCloud::ATTSmokeCloud()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void ATTSmokeCloud::BeginPlay()
{
	Super::BeginPlay();
	FRandomStream Rng(FMath::Rand());
	for (int32 i = 0; i < 9; ++i)
	{
		FPuff Puff;
		Puff.Offset = FVector(Rng.FRandRange(-0.55f, 0.55f), Rng.FRandRange(-0.55f, 0.55f), Rng.FRandRange(-0.35f, 0.25f));
		Puff.Drift = FVector(Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(2.f, 8.f));
		Puff.Scale = Rng.FRandRange(0.7f, 1.15f);
		Puff.Spin = Rng.FRandRange(-12.f, 12.f);
		Puff.Mesh = TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector::ZeroVector, FVector(10.f), FLinearColor::Gray, FRotator::ZeroRotator, false);
		if (Puff.Mesh)
		{
			Puff.Mesh->SetCastShadow(false);
			Puff.Material = TTShapes::MakeSmoke(Puff.Mesh, 0.f);
			if (Puff.Material)
			{
				Puff.Mesh->SetMaterial(0, Puff.Material);
			}
		}
		Puffs.Add(Puff);
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] Smoke released at %s (%d puffs, material %s)"), *GetActorLocation().ToCompactString(), Puffs.Num(),
		(Puffs.Num() > 0 && Puffs[0].Material) ? TEXT("ok") : TEXT("MISSING"));
	if (ATTGameMode* GM = GetTTGameMode(this))
	{
		GM->RegisterSmoke(this);
		GM->ReportNoise(GetActorLocation(), ETTNoise::QuietDoor, GetInstigator());
	}
}

void ATTSmokeCloud::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace SmokeTuning;
	Age += DeltaSeconds;
	const float Grow = FMath::Clamp(Age / GrowTime, 0.f, 1.f);
	const float VisualRadius = MaxRadius * FMath::Sqrt(Grow);
	Radius = VisualRadius;
	const float FadeStart = GrowTime + HoldTime;
	const float Fade = FMath::Clamp((Age - FadeStart) / FadeTime, 0.f, 1.f);
	if (Fade >= 1.f)
	{
		Destroy();
		return;
	}
	// Thinning smoke stops blocking sight before it disappears completely.
	if (Fade > 0.6f)
	{
		Radius *= 1.f - (Fade - 0.6f) / 0.4f;
	}

	const float Opacity = 0.5f * FMath::Clamp(Age / 0.6f, 0.f, 1.f) * (1.f - Fade);
	for (FPuff& Puff : Puffs)
	{
		if (!Puff.Mesh)
		{
			continue;
		}
		Puff.Offset += Puff.Drift * DeltaSeconds / FMath::Max(VisualRadius, 50.f);
		const float Size = FMath::Max(10.f, VisualRadius * 1.25f * Puff.Scale);
		Puff.Mesh->SetRelativeLocation(Puff.Offset * VisualRadius);
		Puff.Mesh->SetRelativeScale3D(FVector(Size / 100.f));
		Puff.Mesh->AddRelativeRotation(FRotator(0.f, Puff.Spin * DeltaSeconds, 0.f));
		if (Puff.Material)
		{
			Puff.Material->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		}
	}
}

bool ATTSmokeCloud::BlocksSegment(const FVector& A, const FVector& B) const
{
	if (Radius < 60.f)
	{
		return false;
	}
	const FVector Center = GetActorLocation();
	return FMath::PointDistToSegment(Center, A, B) < Radius * 0.85f;
}

// ---------------------------------------------------------------------------
// Flash

ATTFlash::ATTFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetCastShadows(false);
	RootComponent = Light;
}

void ATTFlash::Setup(float Lumens, float AttenuationRadius, float Duration, const FLinearColor& Color)
{
	Peak = Lumens;
	Life = FMath::Max(0.02f, Duration);
	Light->SetIntensityUnits(ELightUnits::Lumens);
	Light->SetIntensity(Peak);
	Light->SetAttenuationRadius(AttenuationRadius);
	Light->SetLightColor(Color);
}

void ATTFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float K = 1.f - Age / Life;
	if (K <= 0.f)
	{
		Destroy();
		return;
	}
	Light->SetIntensity(Peak * K * K);
}
