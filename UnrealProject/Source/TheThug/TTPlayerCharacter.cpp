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

THIRD_PARTY_INCLUDES_START
#include "../../../Tools/SurvivalCore/Inventory.h"
#include "../../../Tools/SurvivalCore/SurvivalCore.h"
THIRD_PARTY_INCLUDES_END

namespace ts = thug::survival;

namespace
{
	std::vector<ts::ItemDefinition> Catalog()
	{
		// Slot costs and limits are balance hypotheses (see Tools/SurvivalCore/README.md).
		return {
			ts::ItemDefinition{ "rifle", ts::ItemCategory::Weapon, 3, 1, false },
			ts::ItemDefinition{ "magazine", ts::ItemCategory::Ammunition, 1, 4, true },
			ts::ItemDefinition{ "grenade", ts::ItemCategory::Explosive, 1, 2, true },
			ts::ItemDefinition{ "smoke", ts::ItemCategory::Explosive, 1, 3, true },
			ts::ItemDefinition{ "bottle", ts::ItemCategory::Utility, 1, 3, true },
			ts::ItemDefinition{ "bandage", ts::ItemCategory::Medical, 1, 3, true },
			ts::ItemDefinition{ "medkit", ts::ItemCategory::Medical, 2, 2, true },
		};
	}

	std::string Id(ETTItem Item)
	{
		return std::string(TCHAR_TO_UTF8(TTItemId(Item)));
	}
}

/** Joe's body and backpack, from /Tools/SurvivalCore. */
struct FTTJoeBody
{
	ts::SurvivalCore Body;
	ts::Inventory Bag;

	explicit FTTJoeBody(int32 Capacity)
		: Body(ts::Tuning{})
		, Bag(Capacity, Catalog())
	{
	}
};

namespace PlayerTuning
{
	constexpr float WalkSpeed = 230.f;
	constexpr float SprintSpeed = 470.f;
	constexpr float CrouchSpeed = 125.f;
	constexpr float ExhaustedSpeed = 170.f;
	constexpr float InteractRange = 230.f;
	constexpr float EyeHeight = 64.f;
	constexpr int32 MagazineSize = 10;
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

ATTPlayerCharacter::~ATTPlayerCharacter() = default;

void ATTPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CameraBaseZ = PlayerTuning::EyeHeight;
	HeartTimer = 1.f;
	if (!Body)
	{
		ApplyLoadout(ETTDifficulty::Normal);
	}
}

// ---------------------------------------------------------------------------
// Inventory

void ATTPlayerCharacter::ApplyLoadout(ETTDifficulty Difficulty)
{
	static const int32 Capacity[] = { 14, 11, 9, 8 };
	const int32 D = static_cast<int32>(Difficulty);
	Body = MakeShared<FTTJoeBody>(Capacity[D]);

	// GDD §36: Hard is rifle + 2 magazines + 1 grenade + 2 smoke. Others scale around it.
	struct FLoad { int32 Mags, Grenades, Smokes, Bottles, Bandages; };
	static const FLoad Loads[] = {
		{ 3, 2, 3, 2, 1 },
		{ 2, 1, 2, 2, 0 },
		{ 2, 1, 2, 1, 0 },
		{ 1, 0, 1, 0, 0 },
	};
	const FLoad& L = Loads[D];
	TryAddItem(ETTItem::Rifle, 1);
	if (L.Mags) TryAddItem(ETTItem::Magazine, L.Mags);
	if (L.Grenades) TryAddItem(ETTItem::Grenade, L.Grenades);
	if (L.Smokes) TryAddItem(ETTItem::Smoke, L.Smokes);
	if (L.Bottles) TryAddItem(ETTItem::Bottle, L.Bottles);
	if (L.Bandages) TryAddItem(ETTItem::Bandage, L.Bandages);
	Rounds = Difficulty == ETTDifficulty::Punitive ? 5 : PlayerTuning::MagazineSize;
}

bool ATTPlayerCharacter::CanCarry(ETTItem Item) const
{
	return Body && Body->Bag.CanAdd(Id(Item), 1);
}

bool ATTPlayerCharacter::TryAddItem(ETTItem Item, int32 Count)
{
	return Body && Body->Bag.Add(Id(Item), Count);
}

int32 ATTPlayerCharacter::GetItemCount(ETTItem Item) const
{
	return Body ? Body->Bag.Quantity(Id(Item)) : 0;
}

int32 ATTPlayerCharacter::GetUsedSlots() const
{
	return Body ? static_cast<int32>(Body->Bag.UsedSlots()) : 0;
}

int32 ATTPlayerCharacter::GetCapacity() const
{
	return Body ? Body->Bag.CapacitySlots() : 0;
}

void ATTPlayerCharacter::DropItem(ETTItem Item)
{
	if (!Body || bFrozen || IsHidden() || !Body->Bag.Remove(Id(Item), 1, ts::RemovalReason::Drop))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FVector Loc = GetActorLocation() + GetActorForwardVector() * 60.f;
	Loc.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (ATTPickup* Pickup = GetWorld()->SpawnActor<ATTPickup>(ATTPickup::StaticClass(), Loc, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Params))
	{
		Pickup->Setup(ETTPickupType::Item, Item);
	}
	TTAudio::Play2D(this, ETTSfx::Click, 0.3f);
}

// ---------------------------------------------------------------------------
// Body

int32 ATTPlayerCharacter::GetInjury() const { return Body ? static_cast<int32>(Body->Body.CurrentInjury()) : 0; }
bool ATTPlayerCharacter::IsBleeding() const { return Body && Body->Body.IsBleeding(); }
float ATTPlayerCharacter::GetBloodLoss() const { return Body ? static_cast<float>(Body->Body.BloodLoss()) : 0.f; }
bool ATTPlayerCharacter::IsCollapsed() const { return Body && Body->Body.CurrentEffects().collapsed; }
bool ATTPlayerCharacter::IsDead() const { return Body && Body->Body.CurrentEffects().dead; }

float ATTPlayerCharacter::GetStamina() const
{
	return Body ? static_cast<float>(Body->Body.Stamina() / 100.0) : 1.f;
}

void ATTPlayerCharacter::TakeInjury(int32 Level, const FString& Reason)
{
	if (!Body || IsDead())
	{
		return;
	}
	const int32 Before = GetInjury();
	const ts::Injury Severity = static_cast<ts::Injury>(FMath::Clamp(Level, 1, 3));
	const bool bChanged = Body->Body.ApplyInjury(Severity);
	DamageFlash = 1.f;
	ShakeTime = 0.35f;
	CancelAction();
	UE_LOG(LogTemp, Log, TEXT("[TT] Joe injured: level %d -> %d (%s)"), Before, GetInjury(), *Reason);
	if (!bChanged)
	{
		Message(Reason, 3.f);
		return;
	}
	switch (GetInjury())
	{
	case 1: Message(Reason + TEXT(" Ferimento leve."), 3.5f); break;
	case 2: Message(Reason + TEXT(" Ferido: correr cansa mais e você respira alto."), 4.5f); break;
	case 3: Message(Reason + TEXT(" Ferimento grave, você está sangrando. [H] para fazer um curativo."), 6.f); break;
	default: break;
	}
}

void ATTPlayerCharacter::HunterStrike(const FVector& From)
{
	const int32 Next = FMath::Clamp(FMath::Max(GetInjury() + 1, 2), 1, 3);
	TakeInjury(Next, TEXT("Ele te acertou."));
	TTAudio::Play2D(this, ETTSfx::Punch, 1.f);
	FVector Away = GetActorLocation() - From;
	Away.Z = 0.f;
	LaunchCharacter(Away.GetSafeNormal() * 650.f + FVector(0.f, 0.f, 220.f), true, true);
	ReportNoise(ETTNoise::Door);
}

void ATTPlayerCharacter::UseMedical()
{
	if (!Body || bFrozen || IsDead() || Action != ETTJoeAction::None)
	{
		return;
	}
	const bool bBleeding = IsBleeding();
	const bool bHurt = GetInjury() > 0 || GetBloodLoss() > 0.f;
	if (bBleeding && GetItemCount(ETTItem::Bandage) > 0)
	{
		StartAction(ETTJoeAction::Bandage, 2.2f);
	}
	else if (bHurt && GetItemCount(ETTItem::Medkit) > 0)
	{
		StartAction(ETTJoeAction::Medkit, 3.5f);
	}
	else if (!bHurt)
	{
		Message(TEXT("Você não está ferido."), 2.f);
	}
	else
	{
		Message(bBleeding ? TEXT("Nada para estancar o sangue. Procure ataduras ou um kit médico.") : TEXT("Sem kit médico."), 3.f);
	}
}

void ATTPlayerCharacter::StartAction(ETTJoeAction NewAction, float Duration)
{
	Action = NewAction;
	ActionTime = 0.f;
	ActionDuration = Duration;
	bSprinting = false;
	TTAudio::Play2D(this, ETTSfx::Click, 0.25f);
}

void ATTPlayerCharacter::CancelAction()
{
	Action = ETTJoeAction::None;
	ActionTime = 0.f;
	ActionDuration = 0.f;
}

void ATTPlayerCharacter::UpdateAction(float DeltaSeconds)
{
	if (Action == ETTJoeAction::None)
	{
		return;
	}
	ActionTime += DeltaSeconds;
	if (ActionTime < ActionDuration)
	{
		return;
	}

	const ETTJoeAction Done = Action;
	CancelAction();
	switch (Done)
	{
	case ETTJoeAction::Bandage:
		if (Body->Bag.Remove(Id(ETTItem::Bandage), 1, ts::RemovalReason::Consume))
		{
			Body->Body.Stabilize();
			Message(TEXT("Sangramento estancado. O ferimento continua lá."), 3.f);
		}
		break;
	case ETTJoeAction::Medkit:
		if (Body->Bag.Remove(Id(ETTItem::Medkit), 1, ts::RemovalReason::Consume))
		{
			Body->Body.Treat();
			Message(TEXT("Curativo feito. Você está um pouco melhor."), 3.f);
		}
		break;
	case ETTJoeAction::Reload:
		if (Body->Bag.Remove(Id(ETTItem::Magazine), 1, ts::RemovalReason::Consume))
		{
			Rounds = PlayerTuning::MagazineSize;
			TTAudio::Play2D(this, ETTSfx::Click, 0.5f);
		}
		break;
	default:
		break;
	}
}

void ATTPlayerCharacter::UpdateBody(float DeltaSeconds)
{
	if (!Body || bFrozen)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const bool bWantsSprint = bSprintHeld && !bIsCrouched && MoveInput.Y > 0.1f && CanMove() && !bAiming && Action == ETTJoeAction::None;
	ts::Activity Activity = ts::Activity::Walk;
	if (IsHidden() || Speed < 15.f)
	{
		Activity = ts::Activity::Rest;
	}
	else if (bIsCrouched)
	{
		Activity = ts::Activity::Sneak;
	}
	else if (bWantsSprint)
	{
		Activity = ts::Activity::Sprint;
	}

	bSprinting = bWantsSprint && Body->Body.CurrentEffects().canSprint;

	ts::AdvanceResult Result;
	try
	{
		Result = Body->Body.Advance(DeltaSeconds, Activity);
	}
	catch (...)
	{
		return;
	}

	if (Result.becameCollapsed)
	{
		CancelAction();
		Message(TEXT("Você desabou. Sem tratamento, vai sangrar até morrer. [H] curativo."), 8.f);
		UE_LOG(LogTemp, Log, TEXT("[TT] Joe collapsed (blood loss %.0f)"), GetBloodLoss());
	}
	if (Result.becameDead && !bDeathReported)
	{
		bDeathReported = true;
		if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
		{
			GM->PlayerDied(TEXT("Você sangrou até a morte no chão de um hospital abandonado."));
		}
	}

	// GDD §43: low stamina and injuries make Joe breathe loudly (and the Hunter can hear it).
	const ts::Effects Effects = Body->Body.CurrentEffects();
	const float Breath = static_cast<float>(Effects.breathingLoudnessMultiplier) * (Body->Body.IsExhausted() ? 1.5f : 1.f);
	if (Breath >= 1.5f && !Effects.dead)
	{
		BreathTimer -= DeltaSeconds;
		if (BreathTimer <= 0.f)
		{
			BreathTimer = FMath::Clamp(2.4f / Breath, 0.8f, 1.6f);
			TTAudio::Play2D(this, ETTSfx::Breath, FMath::Clamp(0.15f * Breath, 0.2f, 0.6f));
			ReportNoise(ETTNoise::Breath);
		}
	}
}

// ---------------------------------------------------------------------------
// Tick

void ATTPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FireCooldown -= DeltaSeconds;
	GlassInjuryCooldown -= DeltaSeconds;
	DamageFlash = FMath::Max(0.f, DamageFlash - DeltaSeconds * 1.5f);
	ShakeTime = FMath::Max(0.f, ShakeTime - DeltaSeconds);

	UpdateBody(DeltaSeconds);
	UpdateAction(DeltaSeconds);
	UpdateMovement(DeltaSeconds);
	UpdateFocus();
	UpdateFootsteps(DeltaSeconds);
	UpdateHeartbeat(DeltaSeconds);
	UpdateCamera(DeltaSeconds);

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
	Flashlight->SetVisibility(bFlashlightOn && !IsHidden() && !IsDead());
}

void ATTPlayerCharacter::UpdateCamera(float DeltaSeconds)
{
	CrouchCameraOffset = FMath::FInterpTo(CrouchCameraOffset, 0.f, DeltaSeconds, 10.f);
	CollapseCameraOffset = FMath::FInterpTo(CollapseCameraOffset, IsCollapsed() || IsDead() ? -110.f : 0.f, DeltaSeconds, 3.f);

	const float Speed = GetVelocity().Size2D();
	float BobAmp = 0.f;
	if (!IsHidden() && GetCharacterMovement()->IsMovingOnGround() && Speed > 20.f)
	{
		BobAmp = bSprinting ? 3.2f : (bIsCrouched ? 1.0f : 1.7f);
		if (GetInjury() >= 2)
		{
			BobAmp *= 1.4f; // limping
		}
	}
	const float BobZ = FMath::Sin(BobPhase * 2.f) * BobAmp;
	const float BobY = FMath::Cos(BobPhase) * BobAmp * 0.6f;
	FVector Shake = FVector::ZeroVector;
	if (ShakeTime > 0.f)
	{
		Shake = FVector(0.f, FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * 6.f * (ShakeTime / 0.35f);
	}
	Camera->SetRelativeLocation(FVector(0.f, BobY, CameraBaseZ + CrouchCameraOffset + CollapseCameraOffset + BobZ) + Shake);

	Fov = FMath::FInterpTo(Fov, bAiming && !IsHidden() ? 60.f : 88.f, DeltaSeconds, 10.f);
	Camera->SetFieldOfView(Fov);
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

bool ATTPlayerCharacter::CanAct() const
{
	return !bFrozen && !IsHidden() && !IsCollapsed() && !IsDead();
}

bool ATTPlayerCharacter::CanMove() const
{
	return CanAct() && (Action == ETTJoeAction::None || Action == ETTJoeAction::Reload);
}

void ATTPlayerCharacter::UpdateMovement(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!CanMove())
	{
		bSprinting = false;
		if (!IsHidden() && !bFrozen)
		{
			Move->StopActiveMovement();
		}
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

	const ts::Effects Effects = Body ? Body->Body.CurrentEffects() : ts::Effects{};
	float Base = bSprinting ? PlayerTuning::SprintSpeed : PlayerTuning::WalkSpeed;
	if (Body && Body->Body.IsExhausted())
	{
		Base = PlayerTuning::ExhaustedSpeed;
	}
	if (bAiming || Action == ETTJoeAction::Reload)
	{
		Base = FMath::Min(Base, 150.f);
	}
	const float Multiplier = static_cast<float>(Effects.speedMultiplier);
	Move->MaxWalkSpeed = Base * Multiplier;
	Move->MaxWalkSpeedCrouched = PlayerTuning::CrouchSpeed * Multiplier;

	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MoveInput.X);
}

void ATTPlayerCharacter::UpdateFocus()
{
	Focus = nullptr;
	if (bFrozen || IsDead())
	{
		return;
	}
	if (IsHidden())
	{
		Focus = HiddenIn;
		return;
	}
	if (IsCollapsed())
	{
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
	if (StepDistance < Stride)
	{
		return;
	}
	StepDistance = 0.f;

	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	const ATTHospital* Hospital = GM ? GM->GetHospital() : nullptr;
	if (Hospital && Hospital->IsGlassAt(GetActorLocation()))
	{
		// Broken glass: loud underfoot, and running over it cuts.
		TTAudio::Play2D(this, ETTSfx::Shatter, bIsCrouched ? 0.12f : 0.35f);
		if (!bIsCrouched)
		{
			ReportNoise(ETTNoise::GlassCrunch);
		}
		if (bSprinting && GlassInjuryCooldown <= 0.f)
		{
			GlassInjuryCooldown = 6.f;
			TakeInjury(1, TEXT("Você correu sobre vidro quebrado."));
		}
		return;
	}

	const ETTNoise Kind = bIsCrouched ? ETTNoise::SlowStep : (bSprinting ? ETTNoise::SprintStep : ETTNoise::Step);
	const float Volume = bIsCrouched ? 0.12f : (bSprinting ? 0.55f : 0.28f);
	TTAudio::Play2D(this, ETTSfx::PlayerStep, Volume);
	ReportNoise(Kind);
}

void ATTPlayerCharacter::UpdateHeartbeat(float DeltaSeconds)
{
	const ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	const ATTHunter* Hunter = GM ? GM->GetHunter() : nullptr;
	if (!Hunter || !GM->IsPlaying() || IsDead())
	{
		return;
	}

	const float Dist = FVector::Dist(Hunter->GetActorLocation(), GetActorLocation());
	float Closeness = FMath::Clamp(1.f - Dist / 1900.f, 0.f, 1.f);
	if (Hunter->IsChasing())
	{
		Closeness = FMath::Max(Closeness, 0.85f);
	}
	if (IsBleeding())
	{
		Closeness = FMath::Max(Closeness, 0.35f);
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

void ATTPlayerCharacter::Message(const FString& Text, float Duration) const
{
	if (ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>())
	{
		GM->ShowMessage(Text, Duration);
	}
}

// ---------------------------------------------------------------------------
// Actions

void ATTPlayerCharacter::ToggleFlashlight()
{
	if (bFrozen || IsHidden() || IsDead())
	{
		return;
	}
	bFlashlightOn = !bFlashlightOn;
	TTAudio::Play2D(this, ETTSfx::Click, 0.35f);
}

void ATTPlayerCharacter::Interact()
{
	if (bFrozen || IsDead())
	{
		return;
	}
	if (IsHidden())
	{
		HiddenIn->Interact(this);
	}
	else if (Focus && CanAct())
	{
		Focus->Interact(this);
	}
}

bool ATTPlayerCharacter::Throw(ETTItem Item, int32 ThrowType, float Speed)
{
	if (!CanAct() || Action != ETTJoeAction::None || !Body)
	{
		return false;
	}
	if (!Body->Bag.Remove(Id(Item), 1, ts::RemovalReason::Consume))
	{
		Message(FString::Printf(TEXT("Sem %s."), *TTItemName(Item).ToLower()), 2.f);
		return false;
	}
	const FVector Dir = Camera->GetForwardVector();
	const FVector Start = Camera->GetComponentLocation() + Dir * 45.f - FVector(0.f, 0.f, 10.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATTThrowable* Thrown = GetWorld()->SpawnActor<ATTThrowable>(ATTThrowable::StaticClass(), Start, Dir.Rotation(), Params))
	{
		Thrown->Launch(Dir * Speed + FVector(0.f, 0.f, 220.f), this, static_cast<ETTThrowableType>(ThrowType));
	}
	return true;
}

void ATTPlayerCharacter::ThrowBottle()
{
	Throw(ETTItem::Bottle, static_cast<int32>(ETTThrowableType::Bottle), 1500.f);
}

void ATTPlayerCharacter::ThrowSmoke()
{
	Throw(ETTItem::Smoke, static_cast<int32>(ETTThrowableType::Smoke), 1100.f);
}

void ATTPlayerCharacter::ThrowGrenade()
{
	Throw(ETTItem::Grenade, static_cast<int32>(ETTThrowableType::Grenade), 1250.f);
}

void ATTPlayerCharacter::Reload()
{
	if (!CanAct() || Action != ETTJoeAction::None || GetItemCount(ETTItem::Rifle) == 0)
	{
		return;
	}
	if (Rounds >= PlayerTuning::MagazineSize)
	{
		return;
	}
	if (GetItemCount(ETTItem::Magazine) == 0)
	{
		Message(TEXT("Sem carregadores."), 2.f);
		return;
	}
	StartAction(ETTJoeAction::Reload, 2.2f);
}

void ATTPlayerCharacter::Fire()
{
	if (!CanAct() || FireCooldown > 0.f || Action != ETTJoeAction::None || GetItemCount(ETTItem::Rifle) == 0)
	{
		return;
	}
	if (Rounds <= 0)
	{
		FireCooldown = 0.3f;
		TTAudio::Play2D(this, ETTSfx::Click, 0.6f);
		Message(GetItemCount(ETTItem::Magazine) > 0 ? TEXT("Vazio. [R] recarregar.") : TEXT("Sem munição."), 2.f);
		return;
	}
	--Rounds;
	FireCooldown = 0.35f;

	UWorld* World = GetWorld();
	const float Spread = bAiming ? 0.6f : 2.5f;
	const FVector Dir = FMath::VRandCone(Camera->GetForwardVector(), FMath::DegreesToRadians(Spread));
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Dir * 8000.f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATTFlash* Flash = World->SpawnActor<ATTFlash>(ATTFlash::StaticClass(), Start + Camera->GetForwardVector() * 70.f, FRotator::ZeroRotator, Params))
	{
		Flash->Setup(9000.f, 700.f, 0.07f, FLinearColor(1.f, 0.8f, 0.5f));
	}
	TTAudio::Play2D(this, ETTSfx::Gunshot, 0.9f);
	ReportNoise(ETTNoise::Gunshot);

	// Pawns ignore the Visibility channel, so shots query object types directly.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TTShot), false, this);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FHitResult Hit;
	ATTGameMode* GM = World->GetAuthGameMode<ATTGameMode>();
	if (World->LineTraceSingleByObjectType(Hit, Start, End, Objects, Query))
	{
		if (ATTHunter* Hunter = Cast<ATTHunter>(Hit.GetActor()))
		{
			// GDD §37: it buys time. It does not win the fight.
			Hunter->Stun(1.3f, false);
		}
		else if (GM && GM->GetHospital())
		{
			GM->GetHospital()->SetLampsNear(Hit.ImpactPoint, 170.f, false);
		}
	}

	if (AController* C = GetController())
	{
		FRotator R = C->GetControlRotation();
		R.Pitch += bAiming ? 0.7f : 1.6f;
		R.Yaw += FMath::FRandRange(-0.5f, 0.5f);
		C->SetControlRotation(R);
	}
}

void ATTPlayerCharacter::EnterLocker(ATTLocker* Locker)
{
	if (!Locker || !CanAct())
	{
		UE_LOG(LogTemp, Log, TEXT("[TT] EnterLocker refused (frozen %d, hidden %d)"), bFrozen ? 1 : 0, IsHidden() ? 1 : 0);
		return;
	}

	CancelAction();
	bAiming = false;
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
	if (bIsCrouched || IsCollapsed())
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
		bAiming = false;
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
