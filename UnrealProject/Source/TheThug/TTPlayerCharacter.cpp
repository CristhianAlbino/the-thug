#include "TTPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "TTAudio.h"
#include "TTGameMode.h"
#include "TTHospital.h"
#include "TTHunter.h"
#include "TTInteractables.h"

namespace PlayerTuning
{
	constexpr float WalkSpeed = 230.f;
	constexpr float SprintSpeed = 470.f;
	constexpr float CrouchSpeed = 125.f;
	constexpr float ExhaustedSpeed = 170.f;
	constexpr float SprintSeconds = 4.5f;
	constexpr float RegenSeconds = 7.f;
	constexpr float InteractRange = 230.f;
	constexpr float EyeHeight = 64.f;
}

ATTPlayerCharacter::ATTPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 92.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, PlayerTuning::EyeHeight));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(88.f);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(Camera);
	Flashlight->SetRelativeLocation(FVector(10.f, 14.f, -14.f));
	Flashlight->IntensityUnits = ELightUnits::Lumens;
	Flashlight->Intensity = 420.f;
	Flashlight->AttenuationRadius = 2600.f;
	Flashlight->InnerConeAngle = 11.f;
	Flashlight->OuterConeAngle = 27.f;
	Flashlight->SourceRadius = 2.f;
	Flashlight->LightColor = FColor(255, 238, 212);
	Flashlight->VolumetricScatteringIntensity = 0.7f;
	Flashlight->CastShadows = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = PlayerTuning::WalkSpeed;
	Move->MaxWalkSpeedCrouched = PlayerTuning::CrouchSpeed;
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	Move->SetCrouchedHalfHeight(56.f);
	Move->MaxAcceleration = 1500.f;
	Move->BrakingDecelerationWalking = 1700.f;
	Move->GroundFriction = 8.f;
	Move->bCanWalkOffLedgesWhenCrouching = true;
}

void ATTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CameraBaseZ = PlayerTuning::EyeHeight;
	HeartTimer = 1.f;
}

void ATTPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMovement(DeltaSeconds);
	UpdateFocus();
	UpdateFootsteps(DeltaSeconds);
	UpdateHeartbeat(DeltaSeconds);

	LightTimer -= DeltaSeconds;
	if (LightTimer <= 0.f)
	{
		LightTimer = 0.15f;
		if (const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
		{
			if (const ATTHospital* Hospital = GM->GetHospital())
			{
				LightLevel = Hospital->GetLightLevelAt(GetActorLocation() + FVector(0.f, 0.f, 30.f), this);
			}
		}
	}

	// Rare flashlight stutter: an old torch in a dead hospital.
	FlashlightFlickerTimer -= DeltaSeconds;
	if (bFlashlightOn && !IsHidden())
	{
		if (FlashlightFlickerTimer <= 0.f)
		{
			FlashlightFlickerTimer = FMath::FRandRange(8.f, 25.f);
		}
		const bool bDip = FlashlightFlickerTimer < 0.25f && FMath::FRand() < 0.5f;
		Flashlight->SetIntensity(bDip ? 60.f : 420.f);
	}
	Flashlight->SetVisibility(bFlashlightOn && !IsHidden());

	// Camera: smooth crouch transition and head bob.
	CrouchCameraOffset = FMath::FInterpTo(CrouchCameraOffset, 0.f, DeltaSeconds, 10.f);
	const float Speed = GetVelocity().Size2D();
	float BobAmp = 0.f;
	if (!IsHidden() && GetCharacterMovement()->IsMovingOnGround() && Speed > 20.f)
	{
		BobAmp = bSprinting ? 3.2f : (bIsCrouched ? 1.0f : 1.7f);
	}
	const float BobZ = FMath::Sin(BobPhase * 2.f) * BobAmp;
	const float BobY = FMath::Cos(BobPhase) * BobAmp * 0.6f;
	Camera->SetRelativeLocation(FVector(0.f, BobY, CameraBaseZ + CrouchCameraOffset + BobZ));
}

void ATTPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchCameraOffset += ScaledHalfHeightAdjust;
}

void ATTPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CrouchCameraOffset -= ScaledHalfHeightAdjust;
}

void ATTPlayerCharacter::UpdateMovement(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bFrozen || IsHidden())
	{
		bSprinting = false;
		Stamina = FMath::Min(1.f, Stamina + DeltaSeconds / PlayerTuning::RegenSeconds);
		return;
	}

	const bool bWantsCrouch = bCrouchToggled || bCrouchHeld;
	if (bWantsCrouch && !bIsCrouched)
	{
		Crouch();
	}
	else if (!bWantsCrouch && bIsCrouched)
	{
		UnCrouch();
	}

	bSprinting = bSprintHeld && !bExhausted && !bIsCrouched && MoveInput.Y > 0.1f && Stamina > 0.f;
	if (bSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - DeltaSeconds / PlayerTuning::SprintSeconds);
		StaminaRegenDelay = 0.9f;
		if (Stamina <= 0.f)
		{
			bExhausted = true;
		}
	}
	else
	{
		StaminaRegenDelay -= DeltaSeconds;
		if (StaminaRegenDelay <= 0.f)
		{
			Stamina = FMath::Min(1.f, Stamina + DeltaSeconds / PlayerTuning::RegenSeconds);
		}
		if (bExhausted && Stamina > 0.35f)
		{
			bExhausted = false;
		}
	}

	// GDD §43: low stamina makes Joe breathe loudly.
	if (bExhausted)
	{
		BreathTimer -= DeltaSeconds;
		if (BreathTimer <= 0.f)
		{
			BreathTimer = 1.1f;
			ReportNoise(ETTNoise::Breath);
		}
	}

	Move->MaxWalkSpeed = bSprinting ? PlayerTuning::SprintSpeed : (bExhausted ? PlayerTuning::ExhaustedSpeed : PlayerTuning::WalkSpeed);

	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right, MoveInput.X);
}

void ATTPlayerCharacter::UpdateFocus()
{
	Focus = nullptr;
	if (bFrozen)
	{
		return;
	}
	if (IsHidden())
	{
		Focus = HiddenIn;
		return;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * PlayerTuning::InteractRange;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTInteract), false, this);
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(10.f), Query))
	{
		ATTInteractable* Candidate = Cast<ATTInteractable>(Hit.GetActor());
		if (Candidate && !Candidate->GetPrompt(this).IsEmpty())
		{
			Focus = Candidate;
		}
	}
}

void ATTPlayerCharacter::UpdateFootsteps(float DeltaSeconds)
{
	if (IsHidden() || bFrozen || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	const float Speed = GetVelocity().Size2D();
	if (Speed < 40.f)
	{
		return;
	}

	const float Stride = bIsCrouched ? 110.f : (bSprinting ? 240.f : 185.f);
	BobPhase += Speed * DeltaSeconds / Stride * PI;
	StepDistance += Speed * DeltaSeconds;
	if (StepDistance >= Stride)
	{
		StepDistance = 0.f;
		const ETTNoise Kind = bIsCrouched ? ETTNoise::SlowStep : (bSprinting ? ETTNoise::SprintStep : ETTNoise::Step);
		const float Volume = bIsCrouched ? 0.12f : (bSprinting ? 0.55f : 0.28f);
		TTAudio::Play2D(this, ETTSfx::PlayerStep, Volume);
		ReportNoise(Kind);
	}
}

void ATTPlayerCharacter::UpdateHeartbeat(float DeltaSeconds)
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	const ATTHunter* Hunter = GM ? GM->GetHunter() : nullptr;
	if (!Hunter || !GM->IsPlaying())
	{
		return;
	}

	const float Dist = FVector::Dist(Hunter->GetActorLocation(), GetActorLocation());
	float Closeness = FMath::Clamp(1.f - Dist / 1900.f, 0.f, 1.f);
	if (Hunter->IsChasing())
	{
		Closeness = FMath::Max(Closeness, 0.85f);
	}
	if (Closeness <= 0.f)
	{
		HeartTimer = FMath::Min(HeartTimer, 0.5f);
		return;
	}

	HeartTimer -= DeltaSeconds;
	if (HeartTimer <= 0.f)
	{
		HeartTimer = FMath::Lerp(1.15f, 0.42f, Closeness);
		TTAudio::Play2D(this, ETTSfx::Heartbeat, 0.2f + 0.7f * Closeness);
	}
}

void ATTPlayerCharacter::ReportNoise(ETTNoise Kind)
{
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->ReportNoise(GetActorLocation(), Kind, this);
	}
}

void ATTPlayerCharacter::SetCrouchHeld(bool bHeld)
{
	bCrouchHeld = bHeld;
}

void ATTPlayerCharacter::ToggleCrouch()
{
	bCrouchToggled = !bCrouchToggled;
}

void ATTPlayerCharacter::ToggleFlashlight()
{
	if (bFrozen || IsHidden())
	{
		return;
	}
	bFlashlightOn = !bFlashlightOn;
	TTAudio::Play2D(this, ETTSfx::Click, 0.35f);
}

void ATTPlayerCharacter::Interact()
{
	if (bFrozen)
	{
		return;
	}
	if (IsHidden())
	{
		HiddenIn->Interact(this);
	}
	else if (Focus)
	{
		Focus->Interact(this);
	}
}

void ATTPlayerCharacter::ThrowBottle()
{
	if (bFrozen || IsHidden())
	{
		return;
	}
	ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	if (Bottles <= 0)
	{
		if (GM)
		{
			GM->ShowMessage(TEXT("Sem garrafas para jogar."), 2.f);
		}
		return;
	}

	const FVector Dir = Camera->GetForwardVector();
	const FVector Start = Camera->GetComponentLocation() + Dir * 45.f - FVector(0.f, 0.f, 10.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATTThrowable* Bottle = GetWorld()->SpawnActor<ATTThrowable>(ATTThrowable::StaticClass(), Start, Dir.Rotation(), Params))
	{
		Bottle->Launch(Dir * 1500.f + FVector(0.f, 0.f, 220.f), this);
		--Bottles;
	}
}

void ATTPlayerCharacter::EnterLocker(ATTLocker* Locker)
{
	if (!Locker || bFrozen || IsHidden())
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] EnterLocker refused (frozen %d, hidden %d)"), bFrozen ? 1 : 0, IsHidden() ? 1 : 0);
		return;
	}

	bCrouchToggled = false;
	bCrouchHeld = false;
	if (bIsCrouched)
	{
		UnCrouch();
	}

	HiddenIn = Locker;
	Locker->SetOccupied(true);
	bFlashlightBeforeHiding = bFlashlightOn;
	bFlashlightOn = false;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);

	FVector Inside = Locker->GetInsideLocation();
	Inside.Z += GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 2.f;
	SetActorLocation(Inside, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(0.f, Locker->GetFacingYaw(), 0.f));
	}

	ReportNoise(ETTNoise::Locker);
	UE_LOG(LogTemp, Log, TEXT("[TT] Joe hid in %s"), *Locker->GetId());
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->OnPlayerHid(Locker);
	}
}

void ATTPlayerCharacter::ExitLocker()
{
	ATTLocker* Locker = HiddenIn;
	if (!Locker)
	{
		return;
	}
	HiddenIn = nullptr;
	Locker->SetOccupied(false);
	bFlashlightOn = bFlashlightBeforeHiding;

	FVector Front = Locker->GetFrontLocation();
	Front.Z += GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 2.f;
	SetActorEnableCollision(true);
	SetActorLocation(Front, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	ReportNoise(ETTNoise::Locker);
	UE_LOG(LogTemp, Log, TEXT("[TT] Joe left %s"), *Locker->GetId());
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->OnPlayerLeftLocker(Locker);
	}
}

float ATTPlayerCharacter::GetVisibility() const
{
	if (IsHidden())
	{
		return 0.f;
	}
	float V = 0.22f + 0.78f * LightLevel;
	if (bFlashlightOn)
	{
		V = FMath::Max(V, 0.95f);
	}
	if (bIsCrouched)
	{
		V *= 0.55f;
	}
	const float Speed = GetVelocity().Size2D();
	if (bSprinting)
	{
		V *= 1.25f;
	}
	else if (Speed < 15.f)
	{
		V *= 0.7f;
	}
	return FMath::Clamp(V, 0.f, 1.f);
}

FVector ATTPlayerCharacter::GetEyeLocation() const
{
	return Camera->GetComponentLocation();
}

void ATTPlayerCharacter::SetFrozen(bool bInFrozen)
{
	bFrozen = bInFrozen;
	if (bFrozen)
	{
		MoveInput = FVector2D::ZeroVector;
		bSprintHeld = false;
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void ATTPlayerCharacter::LookAtPoint(const FVector& Target)
{
	if (AController* C = GetController())
	{
		C->SetControlRotation((Target - GetEyeLocation()).Rotation());
	}
}

void ATTPlayerCharacter::TeleportForTest(const FVector& Location, float Yaw)
{
	if (IsHidden())
	{
		ExitLocker();
	}
	FVector Loc = Location;
	Loc.Z += GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 2.f;
	SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(0.f, Yaw, 0.f));
	}
}
