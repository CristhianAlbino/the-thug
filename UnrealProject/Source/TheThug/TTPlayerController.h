#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TTPlayerController.generated.h"

/** Reads keyboard and mouse directly, so the prototype needs no input assets. */
UCLASS()
class THETHUG_API ATTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATTPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	float GetSensitivity() const { return Sensitivity; }

private:
	bool Pressed(const FKey& Key) const { return WasInputKeyJustPressed(Key); }
	bool Down(const FKey& Key) const { return IsInputKeyDown(Key); }
	void UpdateLook();

	float Sensitivity = 1.f;
};
