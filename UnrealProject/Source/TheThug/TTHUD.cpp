#include "TTHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "TTGameMode.h"
#include "TTHunter.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"
#include "TTPlayerController.h"

namespace
{
	ATTGameMode* GetGM(const AActor* Context)
	{
		return Context->GetWorld()->GetAuthGameMode<ATTGameMode>();
	}

	const FLinearColor Pale(0.85f, 0.86f, 0.82f);
	const FLinearColor Dim(0.55f, 0.56f, 0.53f);
	const FLinearColor Blood(0.55f, 0.02f, 0.02f);

	FString FormatTime(float Seconds)
	{
		const int32 Total = FMath::FloorToInt(Seconds);
		return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
	}
}

void ATTHUD::DrawCentered(const FString& Text, float Y, UFont* Font, float Scale, const FLinearColor& Color)
{
	float W = 0.f;
	float H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, Color, (Canvas->ClipX - W) * 0.5f, Y, Font, Scale);
}

void ATTHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	UI = Canvas->ClipY / 1080.f;

	const ATTGameMode* GM = GetGM(this);
	if (!GM)
	{
		return;
	}

	switch (GM->GetPhase())
	{
	case ETTGamePhase::Title: DrawTitle(); break;
	case ETTGamePhase::Playing: DrawPlaying(); break;
	case ETTGamePhase::Paused: DrawPlaying(); DrawPause(); break;
	case ETTGamePhase::Captured: DrawCaptured(); break;
	case ETTGamePhase::Escaped: DrawEscaped(); break;
	}
}

void ATTHUD::DrawTitle()
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.86f), 0.f, 0.f, W, H);

	UFont* Large = GEngine->GetLargeFont();
	UFont* Medium = GEngine->GetMediumFont();

	DrawCentered(TEXT("THE THUG"), H * 0.24f, Large, 3.4f * UI, Pale);
	DrawCentered(TEXT("Você não é o herói. Você não é o caçador. Você é a presa."), H * 0.37f, Medium, 1.3f * UI, Dim);

	const TCHAR* Lines[] = {
		TEXT("WASD  mover          SHIFT  correr          C / CTRL  agachar"),
		TEXT("E  interagir / esconder          F  lanterna          G  jogar garrafa"),
		TEXT("Agachado você faz menos barulho. Luz e corrida te denunciam."),
		TEXT("Ele ouve. Ele lembra. Ele não trapaceia.")
	};
	float Y = H * 0.52f;
	for (const TCHAR* Line : Lines)
	{
		DrawCentered(Line, Y, Medium, 1.05f * UI, Dim);
		Y += 40.f * UI;
	}

	const float Pulse = 0.55f + 0.45f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 2.5f);
	DrawCentered(TEXT("ENTER para começar        ESC para sair"), H * 0.80f, Medium, 1.2f * UI, FLinearColor(Pale.R, Pale.G, Pale.B, Pulse));
	DrawCentered(TEXT("Protótipo M01"), H * 0.94f, Medium, 0.8f * UI, FLinearColor(0.3f, 0.3f, 0.3f));
}

void ATTHUD::DrawPlaying()
{
	const ATTGameMode* GM = GetGM(this);
	const ATTPlayerCharacter* Player = Cast<ATTPlayerCharacter>(GetOwningPawn());
	if (!Player)
	{
		return;
	}

	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UFont* Medium = GEngine->GetMediumFont();
	UFont* Small = GEngine->GetSmallFont();

	// Hidden: darkness of the locker around the slats.
	if (Player->IsHidden())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.3f), 0.f, 0.f, W, H * 0.12f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.3f), 0.f, H * 0.88f, W, H * 0.12f);
		DrawCentered(TEXT("Escondido"), H * 0.86f, Medium, 1.f * UI, FLinearColor(Dim.R, Dim.G, Dim.B, 0.6f));
	}
	else
	{
		const float Dot = 4.f * UI;
		DrawRect(FLinearColor(1.f, 1.f, 1.f, Player->GetFocus() ? 0.8f : 0.25f), (W - Dot) * 0.5f, (H - Dot) * 0.5f, Dot, Dot);
	}

	if (const ATTInteractable* Focus = Player->GetFocus())
	{
		const FString Prompt = Focus->GetPrompt(Player);
		if (!Prompt.IsEmpty())
		{
			DrawCentered(Prompt, H * 0.55f, Medium, 1.f * UI, Pale);
		}
	}

	if (const float A = GM->GetObjectiveAlpha(); A > 0.f)
	{
		DrawText(GM->GetObjective(), FLinearColor(Pale.R, Pale.G, Pale.B, A), 48.f * UI, 40.f * UI, Medium, 1.f * UI);
	}
	if (const float A = GM->GetMessageAlpha(); A > 0.f)
	{
		DrawCentered(GM->GetMessage(), H * 0.72f, Medium, 1.05f * UI, FLinearColor(Pale.R, Pale.G, Pale.B, A));
	}
	if (const float A = GM->GetSubtitleAlpha(); A > 0.f)
	{
		DrawCentered(FString::Printf(TEXT("— %s"), *GM->GetSubtitle()), H * 0.79f, Medium, 1.25f * UI, FLinearColor(0.75f, 0.8f, 0.9f, A));
	}

	// Stamina only appears when it matters.
	const float Stamina = Player->GetStamina();
	if (Stamina < 0.99f)
	{
		const float BarW = 220.f * UI;
		const float X = (W - BarW) * 0.5f;
		const float Y = H - 60.f * UI;
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.08f), X, Y, BarW, 3.f * UI);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.4f), X, Y, BarW * Stamina, 3.f * UI);
	}

	FString Items = FString::Printf(TEXT("Garrafas: %d"), Player->GetBottles());
	if (Player->HasKey())
	{
		Items += TEXT("     Cartão de acesso");
	}
	float TW = 0.f;
	float TH = 0.f;
	GetTextSize(Items, TW, TH, Small, 1.2f * UI);
	DrawText(Items, FLinearColor(Dim.R, Dim.G, Dim.B, 0.7f), W - TW - 40.f * UI, H - 50.f * UI, Small, 1.2f * UI);

	if (GM->IsDebugEnabled())
	{
		if (const ATTHunter* Hunter = GM->GetHunter())
		{
			const FString Debug = FString::Printf(TEXT("Hunter: %s  awareness %.2f  | Joe vis %.2f light %.2f"),
				*Hunter->GetStateName(), Hunter->GetAwareness(), Player->GetVisibility(), Player->GetLightLevel());
			DrawText(Debug, FLinearColor::Yellow, 48.f * UI, H - 90.f * UI, Small, 1.2f * UI);
		}
	}
}

void ATTHUD::DrawPause()
{
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), 0.f, 0.f, W, H);
	UFont* Large = GEngine->GetLargeFont();
	UFont* Medium = GEngine->GetMediumFont();
	DrawCentered(TEXT("PAUSADO"), H * 0.32f, Large, 2.f * UI, Pale);
	DrawCentered(TEXT("ESC  continuar        R  recomeçar        Q  sair"), H * 0.48f, Medium, 1.1f * UI, Dim);

	float Sens = 1.f;
	if (const ATTPlayerController* PC = Cast<ATTPlayerController>(PlayerOwner))
	{
		Sens = PC->GetSensitivity();
	}
	DrawCentered(FString::Printf(TEXT("Sensibilidade do mouse: %.1f   (+ / -)"), Sens), H * 0.56f, Medium, 1.f * UI, Dim);
}

void ATTHUD::DrawCaptured()
{
	const ATTGameMode* GM = GetGM(this);
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float T = GM->GetPhaseTime();

	DrawRect(FLinearColor(Blood.R, Blood.G, Blood.B, FMath::Clamp(T * 0.6f, 0.f, 0.45f)), 0.f, 0.f, W, H);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp((T - 0.8f) * 0.8f, 0.f, 0.85f)), 0.f, 0.f, W, H);

	if (T > 1.2f)
	{
		UFont* Large = GEngine->GetLargeFont();
		UFont* Medium = GEngine->GetMediumFont();
		DrawCentered(TEXT("VOCÊ FOI PEGO"), H * 0.36f, Large, 2.4f * UI, FLinearColor(0.8f, 0.1f, 0.08f));
		DrawCentered(GM->WasCapturedInLocker() ? TEXT("Ele sabia exatamente onde procurar.") : TEXT("Ele estava mais perto do que parecia."),
			H * 0.47f, Medium, 1.1f * UI, Dim);
		DrawCentered(FString::Printf(TEXT("Tempo sobrevivido: %s"), *FormatTime(GM->GetRunTime())), H * 0.53f, Medium, 1.f * UI, Dim);
		DrawCentered(TEXT("R / ENTER  tentar de novo        ESC  sair"), H * 0.66f, Medium, 1.1f * UI, Pale);
	}
}

void ATTHUD::DrawEscaped()
{
	const ATTGameMode* GM = GetGM(this);
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float T = GM->GetPhaseTime();
	DrawRect(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(T * 0.8f, 0.f, 0.95f)), 0.f, 0.f, W, H);

	if (T > 1.2f)
	{
		UFont* Large = GEngine->GetLargeFont();
		UFont* Medium = GEngine->GetMediumFont();
		DrawCentered(TEXT("VOCÊ ESCAPOU"), H * 0.36f, Large, 2.4f * UI, Pale);
		DrawCentered(TEXT("Por enquanto."), H * 0.47f, Medium, 1.1f * UI, Dim);
		DrawCentered(FString::Printf(TEXT("Tempo: %s"), *FormatTime(GM->GetRunTime())), H * 0.53f, Medium, 1.f * UI, Dim);
		DrawCentered(TEXT("R / ENTER  jogar de novo        ESC  sair"), H * 0.66f, Medium, 1.1f * UI, Pale);
	}
}
