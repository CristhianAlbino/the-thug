#include "TTPlayerController.h"

#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TTGameMode.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"

namespace
{
	// Mouse deltas arrive already scaled by the engine's default axis sensitivity (0.07).
	constexpr float LookScale = 1.6f;
}

ATTPlayerController::ATTPlayerController()
{
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void ATTPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	FParse::Value(FCommandLine::Get(), TEXT("ttsens="), Sensitivity);
}

void ATTPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	ATTPlayerCharacter* Joe = Cast<ATTPlayerCharacter>(GetPawn());
	if (!GM)
	{
		return;
	}

	switch (GM->GetPhase())
	{
	case ETTGamePhase::Title:
		if (Pressed(EKeys::One)) GM->SetDifficulty(ETTDifficulty::Easy);
		if (Pressed(EKeys::Two)) GM->SetDifficulty(ETTDifficulty::Normal);
		if (Pressed(EKeys::Three)) GM->SetDifficulty(ETTDifficulty::Hard);
		if (Pressed(EKeys::Four)) GM->SetDifficulty(ETTDifficulty::Punitive);
		if (Pressed(EKeys::Enter) || Pressed(EKeys::SpaceBar) || Pressed(EKeys::LeftMouseButton))
		{
			GM->StartGame();
		}
		else if (Pressed(EKeys::Escape))
		{
			GM->QuitGame();
		}
		return;

	case ETTGamePhase::Paused:
		if (Pressed(EKeys::Escape) || Pressed(EKeys::P))
		{
			GM->TogglePause();
		}
		else if (Pressed(EKeys::Q))
		{
			GM->QuitGame();
		}
		else if (Pressed(EKeys::R))
		{
			GM->RestartRun();
		}
		else if (Pressed(EKeys::Equals) || Pressed(EKeys::Add) || Pressed(EKeys::Up))
		{
			Sensitivity = FMath::Min(4.f, Sensitivity + 0.1f);
		}
		else if (Pressed(EKeys::Hyphen) || Pressed(EKeys::Subtract) || Pressed(EKeys::Down))
		{
			Sensitivity = FMath::Max(0.1f, Sensitivity - 0.1f);
		}
		return;

	case ETTGamePhase::Captured:
	case ETTGamePhase::Escaped:
		if (GM->GetPhaseTime() > 1.2f)
		{
			if (Pressed(EKeys::R) || Pressed(EKeys::Enter))
			{
				GM->RestartRun();
			}
			else if (Pressed(EKeys::Escape))
			{
				GM->QuitGame();
			}
		}
		return;

	case ETTGamePhase::Playing:
		break;
	}

	if (Joe && Joe->IsViewingCameras())
	{
		if (Pressed(EKeys::A) || Pressed(EKeys::Left) || Pressed(EKeys::Q))
		{
			Joe->CycleCamera(-1);
		}
		if (Pressed(EKeys::D) || Pressed(EKeys::Right))
		{
			Joe->CycleCamera(1);
		}
		if (Pressed(EKeys::E) || Pressed(EKeys::Escape) || Pressed(EKeys::S))
		{
			Joe->StopCameraView();
		}
		Joe->SetMoveInput(FVector2D::ZeroVector);
		return;
	}
	if (Pressed(EKeys::Escape) || Pressed(EKeys::P))
	{
		GM->TogglePause();
		return;
	}
	if (!Joe)
	{
		return;
	}

	FVector2D Move = FVector2D::ZeroVector;
	if (Down(EKeys::W) || Down(EKeys::Up)) Move.Y += 1.f;
	if (Down(EKeys::S) || Down(EKeys::Down)) Move.Y -= 1.f;
	if (Down(EKeys::D) || Down(EKeys::Right)) Move.X += 1.f;
	if (Down(EKeys::A) || Down(EKeys::Left)) Move.X -= 1.f;
	Joe->SetMoveInput(Move.GetSafeNormal());
	Joe->SetSprintHeld(Down(EKeys::LeftShift));
	Joe->SetCrouchHeld(Down(EKeys::LeftControl));

	if (Pressed(EKeys::C))
	{
		Joe->ToggleCrouch();
	}
	if (Pressed(EKeys::F))
	{
		Joe->ToggleFlashlight();
	}
	if (Pressed(EKeys::E))
	{
		Joe->Interact();
	}
	const bool bInventory = Down(EKeys::Tab);
	Joe->SetInventoryOpen(bInventory);
	if (bInventory)
	{
		static const FKey DropKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight };
		for (int32 i = 0; i < UE_ARRAY_COUNT(DropKeys); ++i)
		{
			if (Pressed(DropKeys[i]))
			{
				Joe->DropItem(static_cast<ETTItem>(i));
			}
		}
	}
	else
	{
		Joe->SetAiming(Down(EKeys::RightMouseButton));
		if (Pressed(EKeys::LeftMouseButton))
		{
			Joe->Fire();
		}
	}
	if (Pressed(EKeys::R))
	{
		Joe->Reload();
	}
	if (Pressed(EKeys::G))
	{
		Joe->ThrowBottle();
	}
	if (Pressed(EKeys::T))
	{
		Joe->ThrowSmoke();
	}
	if (Pressed(EKeys::Q))
	{
		Joe->ThrowGrenade();
	}
	if (Pressed(EKeys::H))
	{
		Joe->UseMedical();
	}
	if (Pressed(EKeys::Z))
	{
		Joe->GiveAmmoToFocus();
	}
	if (Pressed(EKeys::X))
	{
		Joe->RequestFollowFromFocus();
	}

	UpdateLook();
}

void ATTPlayerController::UpdateLook()
{
	float DX = 0.f;
	float DY = 0.f;
	GetInputMouseDelta(DX, DY);
	if (DX == 0.f && DY == 0.f)
	{
		return;
	}

	const float Scale = LookScale * Sensitivity;
	FRotator Rot = GetControlRotation();
	Rot.Yaw = FRotator::NormalizeAxis(Rot.Yaw + DX * Scale);
	Rot.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rot.Pitch) + DY * Scale, -80.f, 80.f);
	Rot.Roll = 0.f;

	// Inside a locker Joe can only peek through the slats.
	if (const ATTPlayerCharacter* Joe = Cast<ATTPlayerCharacter>(GetPawn()))
	{
		if (const ATTLocker* Locker = Joe->GetHiddenLocker())
		{
			const float Base = Locker->GetFacingYaw();
			const float Rel = FMath::Clamp(FRotator::NormalizeAxis(Rot.Yaw - Base), -50.f, 50.f);
			Rot.Yaw = Base + Rel;
			Rot.Pitch = FMath::Clamp(Rot.Pitch, -25.f, 20.f);
		}
	}
	SetControlRotation(Rot);
}
