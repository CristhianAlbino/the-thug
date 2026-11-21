#include "TTHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "TTCompanion.h"
#include "TTGameMode.h"
#include "TTHospital.h"
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

void ATTHUD::DrawCentered(const FString& Text, float Y, UFont* Font, float Scale, const FLinearColor& Color, bool bBackdrop)
{
	float W = 0.f;
	float H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	if (bBackdrop)
	{
		const float Pad = 10.f * UI;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f * Color.A), (Canvas->ClipX - W) * 0.5f - Pad, Y - Pad * 0.4f, W + 2.f * Pad, H + Pad * 0.8f);
	}
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
		TEXT("WASD  mover      SHIFT  correr      C / CTRL  agachar      E  interagir / esconder      F  lanterna"),
		TEXT("BOTÃO DIR.  mirar      BOTÃO ESQ.  atirar      R  recarregar      H  curativo      TAB  inventário"),
		TEXT("G  garrafa (distração)      T  fumaça (ele não vê através)      Q  granada"),
		TEXT("Olhando para um companheiro:  E  falar      Z  dar carregador      X  vem comigo / fica"),
		TEXT("Atirar compra tempo e entrega sua posição. Ele ouve. Ele lembra. Ele não trapaceia.")
	};
	float Y = H * 0.52f;
	for (const TCHAR* Line : Lines)
	{
		DrawCentered(Line, Y, Medium, 1.05f * UI, Dim);
		Y += 40.f * UI;
	}

	const ATTGameMode* GM = GetGM(this);
	const FString Diff = FString::Printf(TEXT("Dificuldade:  %s        (1 Fácil   2 Normal   3 Difícil   4 Punitivo)"), *TTDifficultyName(GM->GetDifficulty()));
	DrawCentered(Diff, H * 0.72f, Medium, 1.05f * UI, Pale);

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

	if (Player->IsViewingCameras())
	{
		const ATTHospital* Hospital = GM->GetHospital();
		const int32 Index = Player->GetCameraIndex();
		const bool bBroken = Hospital && Hospital->IsSecurityCameraBroken(Index);
		const float T = GetWorld()->GetRealTimeSeconds();
		// Old CRT: scanlines, noise, the occasional dropped frame.
		for (int32 Line = 0; Line < 60; ++Line)
		{
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.18f), 0.f, Line * H / 60.f, W, H / 180.f);
		}
		if (bBroken || FMath::Fmod(T, 7.f) < 0.12f)
		{
			DrawRect(FLinearColor(0.04f, 0.04f, 0.04f, bBroken ? 1.f : 0.7f), 0.f, 0.f, W, H);
			for (int32 k = 0; k < 220; ++k)
			{
				const float G = FMath::FRand();
				DrawRect(FLinearColor(G, G, G, 0.5f), FMath::FRand() * W, FMath::FRand() * H, FMath::FRandRange(4.f, 40.f) * UI, 2.f * UI);
			}
			if (bBroken)
			{
				DrawCentered(TEXT("SEM SINAL"), H * 0.46f, GEngine->GetLargeFont(), 1.6f * UI, FLinearColor(0.8f, 0.8f, 0.8f));
			}
		}
		const FString Name = Hospital && Hospital->GetSecurityCameraNames().IsValidIndex(Index) ? Hospital->GetSecurityCameraNames()[Index] : FString();
		DrawText(Name, FLinearColor(0.85f, 0.9f, 0.85f), 48.f * UI, 40.f * UI, Medium, 1.1f * UI);
		if (FMath::Fmod(T, 1.f) < 0.6f)
		{
			DrawRect(FLinearColor(0.8f, 0.05f, 0.05f), W - 110.f * UI, 46.f * UI, 14.f * UI, 14.f * UI);
			DrawText(TEXT("REC"), FLinearColor(0.85f, 0.85f, 0.85f), W - 90.f * UI, 40.f * UI, Medium, 1.f * UI);
		}
		DrawCentered(TEXT("A / D  trocar câmera        E  sair dos monitores"), H - 70.f * UI, Medium, 1.f * UI, Pale, true);
		return;
	}

	if (GM->IsKeypadActive())
	{
		const float PW = 380.f * UI;
		const float PH = 250.f * UI;
		const float PX = (W - PW) * 0.5f;
		const float PY = (H - PH) * 0.5f;
		DrawRect(FLinearColor(0.02f, 0.02f, 0.02f, 0.92f), PX, PY, PW, PH);
		DrawCentered(TEXT("COFRE"), PY + 20.f * UI, Medium, 1.1f * UI, Dim);
		FString Shown;
		const FString& Entry = GM->GetKeypadEntry();
		for (int32 i = 0; i < 6; ++i)
		{
			Shown += i < Entry.Len() ? FString::Chr(Entry[i]) : FString(TEXT("_"));
			Shown += TEXT(" ");
		}
		DrawCentered(Shown, PY + 80.f * UI, GEngine->GetLargeFont(), 1.8f * UI, FLinearColor(0.3f, 1.f, 0.4f));
		DrawCentered(TEXT("0-9 digitar    Backspace apagar    Enter confirmar    E sair"), PY + PH - 50.f * UI, Small, 1.1f * UI, Dim);
	}

	if (Player->GetAction() == ETTJoeAction::Crawl || Player->GetAction() == ETTJoeAction::Ride)
	{
		const float K = Player->GetActionProgress();
		const float Fade = FMath::Clamp(FMath::Min(K / 0.25f, (1.f - K) / 0.25f), 0.f, 1.f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.97f * Fade), 0.f, 0.f, W, H);
	}

	// Injury: the edges of the screen darken red, pulsing while bleeding.
	const int32 Injury = Player->GetInjury();
	float Hurt = Injury == 1 ? 0.07f : Injury == 2 ? 0.16f : Injury == 3 ? 0.26f : 0.f;
	if (Player->IsBleeding())
	{
		Hurt += 0.08f * (0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 4.f));
	}
	Hurt = FMath::Max(Hurt, Player->GetDamageFlash() * 0.45f);
	if (Hurt > 0.f)
	{
		for (int32 Band = 0; Band < 6; ++Band)
		{
			const float A = Hurt * (1.f - Band / 6.f);
			const float T = (16.f + Band * 14.f) * UI;
			const float O = Band * 14.f * UI;
			const FLinearColor Red(0.45f, 0.f, 0.f, A * 0.5f);
			DrawRect(Red, O, O, W - 2.f * O, T * 0.5f);
			DrawRect(Red, O, H - O - T * 0.5f, W - 2.f * O, T * 0.5f);
			DrawRect(Red, O, O, T * 0.5f, H - 2.f * O);
			DrawRect(Red, W - O - T * 0.5f, O, T * 0.5f, H - 2.f * O);
		}
	}
	if (Player->IsCollapsed())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, W, H);
	}

	// Hidden: darkness of the locker around the slats.
	if (Player->IsHidden())
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.3f), 0.f, 0.f, W, H * 0.12f);
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.3f), 0.f, H * 0.88f, W, H * 0.12f);
		DrawCentered(TEXT("Escondido"), H * 0.86f, Medium, 1.f * UI, FLinearColor(Dim.R, Dim.G, Dim.B, 0.6f));
	}
	else
	{
		const float Dot = (Player->IsAiming() ? 3.f : 4.f) * UI;
		DrawRect(FLinearColor(1.f, 1.f, 1.f, Player->GetFocus() || Player->IsAiming() ? 0.8f : 0.25f), (W - Dot) * 0.5f, (H - Dot) * 0.5f, Dot, Dot);
	}

	if (const ATTCompanion* Companion = Player->GetFocusCompanion())
	{
		DrawCentered(Companion->GetPrompt(Player), H * 0.55f, Medium, 1.f * UI, Pale, true);
	}
	else if (const ATTInteractable* Focus = Player->GetFocus())
	{
		const FString Prompt = Focus->GetPrompt(Player);
		if (!Prompt.IsEmpty())
		{
			DrawCentered(Prompt, H * 0.55f, Medium, 1.f * UI, Pale, true);
		}
	}

	if (const float A = GM->GetObjectiveAlpha(); A > 0.f)
	{
		DrawText(GM->GetObjective(), FLinearColor(Pale.R, Pale.G, Pale.B, A), 48.f * UI, 40.f * UI, Medium, 1.f * UI);
	}
	if (const float A = GM->GetMessageAlpha(); A > 0.f)
	{
		DrawCentered(GM->GetMessage(), H * 0.72f, Medium, 1.05f * UI, FLinearColor(Pale.R, Pale.G, Pale.B, A), true);
	}
	if (const float A = GM->GetSubtitleAlpha(); A > 0.f)
	{
		DrawCentered(FString::Printf(TEXT("— %s"), *GM->GetSubtitle()), H * 0.79f, Medium, 1.25f * UI, FLinearColor(0.75f, 0.8f, 0.9f, A), true);
	}

	if (const float Heli = GM->GetHelicopterSecondsLeft(); Heli >= 0.f)
	{
		const int32 S = FMath::CeilToInt(Heli);
		const FString Text = FString::Printf(TEXT("%s  %d:%02d"), GM->IsHelicopterHere() ? TEXT("HELICÓPTERO NO TELHADO") : TEXT("HELICÓPTERO A CAMINHO"), S / 60, S % 60);
		DrawCentered(Text, 40.f * UI, Medium, 1.1f * UI, FLinearColor(1.f, 0.85f, 0.4f), true);
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

	// Action in progress (bandage, medkit, reload).
	if (Player->GetAction() != ETTJoeAction::None)
	{
		const TCHAR* Label = Player->GetAction() == ETTJoeAction::Bandage ? TEXT("Enfaixando...")
			: Player->GetAction() == ETTJoeAction::Medkit ? TEXT("Fazendo curativo...")
			: Player->GetAction() == ETTJoeAction::Pry ? TEXT("Arrancando as tábuas... (barulho!)")
			: Player->GetAction() == ETTJoeAction::Crawl ? TEXT("Rastejando...")
			: Player->GetAction() == ETTJoeAction::Ride ? TEXT("O elevador range entre os andares...") : TEXT("Recarregando...");
		const float BarW = 260.f * UI;
		const float X = (W - BarW) * 0.5f;
		const float Y = H * 0.62f;
		DrawCentered(Label, Y - 34.f * UI, Medium, 0.95f * UI, Pale);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.1f), X, Y, BarW, 4.f * UI);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.6f), X, Y, BarW * Player->GetActionProgress(), 4.f * UI);
	}

	if (Injury > 0 || Player->GetBloodLoss() > 0.f)
	{
		static const TCHAR* Names[] = { TEXT(""), TEXT("Ferimento leve"), TEXT("Ferimento médio"), TEXT("Ferimento grave") };
		FString Status = Names[FMath::Clamp(Injury, 0, 3)];
		if (Player->IsBleeding())
		{
			Status += FString::Printf(TEXT("  —  sangrando (%.0f%%)  [H]"), Player->GetBloodLoss());
		}
		DrawText(Status, FLinearColor(0.85f, 0.35f, 0.3f, 0.85f), 48.f * UI, H - 50.f * UI, Small, 1.2f * UI);
	}

	FString Items;
	if (Player->GetItemCount(ETTItem::Rifle) > 0)
	{
		Items += FString::Printf(TEXT("Fuzil %d | +%d     "), Player->GetRounds(), Player->GetItemCount(ETTItem::Magazine));
	}
	Items += FString::Printf(TEXT("Granada %d   Fumaça %d   Garrafa %d   Atadura %d   Kit %d"),
		Player->GetItemCount(ETTItem::Grenade), Player->GetItemCount(ETTItem::Smoke), Player->GetItemCount(ETTItem::Bottle),
		Player->GetItemCount(ETTItem::Bandage), Player->GetItemCount(ETTItem::Medkit));
	if (Player->HasKey())
	{
		Items += TEXT("     Cartão");
	}
	float TW = 0.f;
	float TH = 0.f;
	GetTextSize(Items, TW, TH, Small, 1.2f * UI);
	DrawText(Items, FLinearColor(Dim.R, Dim.G, Dim.B, 0.7f), W - TW - 40.f * UI, H - 50.f * UI, Small, 1.2f * UI);

	if (Player->IsInventoryOpen())
	{
		const float PanelW = 520.f * UI;
		const float PanelH = 430.f * UI;
		const float PX = W - PanelW - 60.f * UI;
		const float PY = (H - PanelH) * 0.5f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), PX, PY, PanelW, PanelH);
		DrawText(FString::Printf(TEXT("INVENTÁRIO      espaço %d / %d"), Player->GetUsedSlots(), Player->GetCapacity()), Pale, PX + 24.f * UI, PY + 18.f * UI, Medium, 1.f * UI);
		for (int32 i = 0; i <= static_cast<int32>(ETTItem::Batteries); ++i)
		{
			const ETTItem Item = static_cast<ETTItem>(i);
			const int32 Count = Player->GetItemCount(Item);
			const FLinearColor C = Count > 0 ? Pale : FLinearColor(0.35f, 0.35f, 0.35f);
			DrawText(FString::Printf(TEXT("%d   %s"), i + 1, *TTItemName(Item)), C, PX + 24.f * UI, PY + (64.f + i * 34.f) * UI, Medium, 0.95f * UI);
			DrawText(FString::Printf(TEXT("x%d"), Count), C, PX + PanelW - 80.f * UI, PY + (64.f + i * 34.f) * UI, Medium, 0.95f * UI);
		}
		DrawText(TEXT("Segure TAB e aperte 2-9 para largar um item no chão (o fuzil fica)."), Dim, PX + 24.f * UI, PY + PanelH - 36.f * UI, Small, 1.1f * UI);

		// Joe's notes and the time (GDD: no map, information is a resource).
		const float NX = 40.f * UI;
		float NY = 120.f * UI;
		DrawText(FString::Printf(TEXT("%s   ANOTAÇÕES"), *GM->GetClockText()), Pale, NX, NY - 40.f * UI, Medium, 1.f * UI);
		if (GM->GetNotes().Num() == 0)
		{
			DrawText(TEXT("Nada ainda. Leia papéis, livros, etiquetas."), Dim, NX, NY, Small, 1.1f * UI);
		}
		for (const FString& Note : GM->GetNotes())
		{
			DrawText(TEXT("· ") + Note, Dim, NX, NY, Small, 1.1f * UI);
			NY += 26.f * UI;
		}
	}

	if (GM->IsDebugEnabled())
	{
		if (const ATTHunter* Hunter = GM->GetHunter())
		{
			const FString Debug = FString::Printf(TEXT("Hunter: %s  awareness %.2f  aggression %.2f  %s | Joe vis %.2f light %.2f"),
				*Hunter->GetStateName(), Hunter->GetAwareness(), Hunter->GetAggression(), *Hunter->GetMemorySummary(),
				Player->GetVisibility(), Player->GetLightLevel());
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
	DrawCentered(FString::Printf(TEXT("%s"), *GetGM(this)->GetClockText()), H * 0.41f, Medium, 1.f * UI, Dim);
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
		DrawCentered(GM->GetEndTitle(), H * 0.36f, Large, 2.4f * UI, FLinearColor(0.8f, 0.1f, 0.08f));
		DrawCentered(GM->GetEndSubtitle(), H * 0.47f, Medium, 1.1f * UI, Dim);
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
		DrawCentered(GM->GetEndSubtitle(), H * 0.47f, Medium, 1.1f * UI, Dim);
		DrawCentered(FString::Printf(TEXT("Tempo: %s"), *FormatTime(GM->GetRunTime())), H * 0.53f, Medium, 1.f * UI, Dim);
		DrawCentered(TEXT("R / ENTER  jogar de novo        ESC  sair"), H * 0.66f, Medium, 1.1f * UI, Pale);
	}
}
