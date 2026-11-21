#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TTHUD.generated.h"

class UFont;

/** Minimal HUD (GDD §100: no screen full of HUD). Text only, drawn with engine fonts. */
UCLASS()
class THETHUG_API ATTHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCentered(const FString& Text, float Y, UFont* Font, float Scale, const FLinearColor& Color);
	void DrawTitle();
	void DrawPlaying();
	void DrawPause();
	void DrawCaptured();
	void DrawEscaped();

	float UI = 1.f;
};
