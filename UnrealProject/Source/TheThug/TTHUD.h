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
	void DrawCentered(const FString& Text, float Y, UFont* Font, float Scale, const FLinearColor& Color, bool bBackdrop = false);
	void DrawTitle();
	void DrawPlaying();
	void DrawPause();
	void DrawCaptured();
	void DrawEscaped();
	void DrawNarrative();
	void DrawEpilogue(float Top);
	/** Word-wrapped text; returns the Y below the last line. */
	float DrawWrapped(const FString& Text, float X, float Y, float MaxWidth, UFont* Font, float Scale, const FLinearColor& Color);

	float UI = 1.f;
};
