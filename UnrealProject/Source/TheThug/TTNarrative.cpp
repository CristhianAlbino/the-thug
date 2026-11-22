#include "TTNarrative.h"

#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TTAudio.h"
#include "TTCompanion.h"
#include "TTGameMode.h"
#include "TTHospital.h"
#include "TTHunter.h"
#include "TTInteractables.h"
#include "TTPlayerCharacter.h"
#include "TTShapes.h"

THIRD_PARTY_INCLUDES_START
#include "../../../Tools/NarrativeCore/NarrativeCore.h"
#include "../../../Tools/HunterCore/RunRules.h"
THIRD_PARTY_INCLUDES_END

#include <string>

namespace tn = thug::narrative;
namespace tr = thug::rules;

/** Story memory: what played (scene ids, "N26:roof", J1...) and what is known (facts). */
struct FTTNarrativeState
{
	TSet<FString> Seen;
	TSet<FString> Facts;
	TMap<FString, int32> VisitCount;
	FString RouteFailure;
	FString NodeOverride;
	TSet<FString> ChargesArmed;
};

struct FTTNarrativeFacts
{
	tn::Facts F;
};

namespace
{
	const TCHAR* JoeName = TEXT("Joe");

	std::string Std(const FString& S)
	{
		return std::string(TCHAR_TO_UTF8(*S));
	}

	/** "no 3º andar"... for epilogues: where Joe last saw someone. */
	FString PlaceOfNode(const FString& Node)
	{
		if (Node.StartsWith(TEXT("F4"))) return TEXT("no 4º andar");
		if (Node.StartsWith(TEXT("F3"))) return TEXT("no 3º andar");
		if (Node.StartsWith(TEXT("F2"))) return TEXT("no 2º andar");
		if (Node.StartsWith(TEXT("F1"))) return TEXT("no 1º andar");
		if (Node.StartsWith(TEXT("B1"))) return TEXT("no subsolo");
		if (Node.StartsWith(TEXT("G_"))) return TEXT("na garagem");
		if (Node.StartsWith(TEXT("T_"))) return TEXT("nos túneis");
		if (Node.StartsWith(TEXT("R_"))) return TEXT("no telhado");
		return TEXT("em algum corredor");
	}

	struct FDoc
	{
		const TCHAR* Id;
		const TCHAR* Title;
		const TCHAR* Body;
		const TCHAR* Note;
	};

	// Campaign DOC06-DOC16 (provisional wording, for review with Cristhian; DOC01-05 are the route stations).
	const FDoc Docs[] = {
		{ TEXT("DOC06"), TEXT("Croqui a lápis — casa de bombas"),
			TEXT("Linha pluvial antiga passa sob a casa de bombas. Grade de inspeção no piso, canto da sala.\nSaída: ? — o desenho termina num borrão de umidade.\nEscrito na margem: \"siga a corrente de ar, não a água\"."),
			TEXT("Croqui: grade no piso da casa de bombas (subsolo, sudeste) leva a uma galeria pluvial. Seguir o ar, não a água.") },
		{ TEXT("DOC07"), TEXT("Ordem de isolamento — 3º andar"),
			TEXT("Por determinação da direção, o quarto de isolamento da ala oeste permanece lacrado.\nAcesso apenas pela sala de utilidades. Porta pesada, sem visor.\nO cômodo não consta mais da planta de evacuação."),
			TEXT("Isolamento esquecido: 3º andar, ala oeste, pela sala de utilidades. Ninguém lembra que existe.") },
		{ TEXT("DOC08"), TEXT("Registro de chamadas — rádio da emergência"),
			TEXT("03/11 — rádio sem resposta. Pilhas fracas. Trocar (servidores, 2º).\n05/11 — pilhas novas, só chiado. O transmissor das câmeras derruba a frequência.\nDesligar o transmissor ANTES de chamar."),
			TEXT("Rádio: pilhas nos servidores (2º); desligar o transmissor das câmeras antes de chamar.") },
		{ TEXT("DOC09"), TEXT("Bilhete colado no monitor"),
			TEXT("Relógio andando no canto = ao vivo.\nMesmo reflexo passando duas vezes = loop.\nA da clínica perde quadro. A da garagem: NÃO confie."),
			TEXT("Câmeras: relógio andando = ao vivo; reflexo repetido = gravação.") },
		{ TEXT("DOC10"), TEXT("Nota da clínica"),
			TEXT("Paciente do leito 4 transferido pelo anexo. A janela leste do 3º dá para a passarela do prédio vizinho.\nJanela pregada depois da \"saída não autorizada\" de setembro."),
			TEXT("Janela leste do 3º andar dá para o prédio vizinho. Pregada.") },
		{ TEXT("DOC11"), TEXT("Ocorrência de porta — servidores"),
			TEXT("Em falha de energia, as portas eletrônicas do 2º travam fechadas.\nPortas manuais continuam abrindo. O elevador para onde estiver."),
			TEXT("Sem energia: portas eletrônicas travam, manuais abrem, elevador para.") },
		{ TEXT("DOC12"), TEXT("Boletim de entrada policial"),
			TEXT("Quarteirão isolado por ordem do comissário.\nNinguém entra, ninguém sai até o amanhecer.\nCruzar o cordão = prisão imediata."),
			TEXT("A polícia fechou o quarteirão. Sair do prédio não é estar livre.") },
		{ TEXT("DOC13"), TEXT("Formulário de transferência — arquivo"),
			TEXT("Transferência de paciente.\nOrigem: ████████████ (raspado à faca).\nNo rodapé, um carimbo meio apagado: \"ASILO ARK...\".\nDestino: Ala Clínica — Anexo Leste."),
			TEXT("Um formulário antigo com carimbo de um asilo. Este hospital já foi outra coisa.") },
		{ TEXT("DOC14"), TEXT("Placa reaproveitada"),
			TEXT("Sob a placa \"CLÍNICA\", a tinta antiga ainda aparece em relevo:\n\"PAVILHÃO B — CONTENÇÃO\"."),
			TEXT("Placa antiga no 3º: \"Pavilhão B — Contenção\".") },
		{ TEXT("DOC15"), TEXT("Inventário antigo — almoxarifado"),
			TEXT("Itens AK-1197 a AK-1240: correias de contenção, sedativos, \"trava de leito, modelo psiquiátrico\".\nNenhum patrimônio deste hospital começa com AK."),
			TEXT("Inventário com numeração AK. Material de contenção.") },
		{ TEXT("DOC16"), TEXT("Fita arquivada — CAM_F4_WEST"),
			TEXT("Sem som. A imagem treme.\n02:41 — corredor oeste do 4º. Uma figura escura arrasta um homem pelo colarinho.\nNo pulso dele, a luz do corredor pega um relógio. Riscado.\n02:43 — a figura para. Um golpe. Outro. O homem não se mexe mais.\nA figura olha direto para a câmera. A fita acaba."),
			TEXT("Fita da câmera do 4º (02:41): ele levou o Thomas. E não o levou para a polícia.") },
	};

	const FDoc* FindDoc(const FString& Id)
	{
		for (const FDoc& Doc : Docs)
		{
			if (Id == Doc.Id)
			{
				return &Doc;
			}
		}
		return nullptr;
	}
}

// ===========================================================================
// Story figures

ATTStoryFigure::ATTStoryFigure()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 92.f);
	GetCharacterMovement()->MaxWalkSpeed = 220.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ATTStoryFigure::Setup(ETTFigure InKind)
{
	Kind = InKind;
	USceneComponent* Root = GetCapsuleComponent();
	const FLinearColor Skin(0.55f, 0.42f, 0.34f);
	if (Kind == ETTFigure::Thomas)
	{
		// Confirmed direction: very dark skin, muscular, 1.97 m, dreads and a big beard.
		const FLinearColor Jacket(0.12f, 0.2f, 0.12f);
		if (TTShapes::SetupMannequin(this, false, FLinearColor(0.035f, 0.02f, 0.014f), Jacket, 1.09f, 0.65f))
		{
			for (int32 i = 0; i < 7; ++i)
			{
				const float A = -60.f + i * 20.f;
				const FVector Off(-8.f - FMath::Cos(FMath::DegreesToRadians(A)) * 6.f, FMath::Sin(FMath::DegreesToRadians(A)) * 10.f, 72.f);
				TTShapes::AddPart(this, Root, TTShapes::Cylinder(), Off, FVector(4.f, 4.f, 30.f), FLinearColor(0.02f, 0.015f, 0.01f), FRotator(12.f, 0.f, 0.f), false);
			}
			TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(9.f, 0.f, 70.f), FVector(14.f, 14.f, 16.f), FLinearColor(0.03f, 0.02f, 0.015f), FRotator::ZeroRotator, false);
		}
		else
		{
			TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, -11.f, -48.f), FVector(18.f, 18.f, 86.f), FLinearColor(0.1f, 0.1f, 0.12f), FRotator::ZeroRotator, false);
			TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 11.f, -48.f), FVector(18.f, 18.f, 86.f), FLinearColor(0.1f, 0.1f, 0.12f), FRotator::ZeroRotator, false);
			TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 28.f), FVector(32.f, 50.f, 70.f), Jacket, FRotator::ZeroRotator, false);
			TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(2.f, 0.f, 76.f), FVector(24.f, 22.f, 27.f), Skin, FRotator::ZeroRotator, false);
		}
		// The scratched watch the morgue scene will recognise.
		TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(10.f, 30.f, 8.f), FVector(7.f, 7.f, 3.f), FLinearColor(0.7f, 0.65f, 0.5f), FRotator(90.f, 0.f, 0.f), false);
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
		if (!TTShapes::SetupMannequin(this, false, FLinearColor(0.04f, 0.14f, 0.05f), FLinearColor(0.45f, 0.05f, 0.04f), 0.95f, 0.5f))
		{
			TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, -10.f, -50.f), FVector(16.f, 16.f, 82.f), FLinearColor(0.05f, 0.15f, 0.05f), FRotator::ZeroRotator, false);
			TTShapes::AddPart(this, Root, TTShapes::Cylinder(), FVector(0.f, 10.f, -50.f), FVector(16.f, 16.f, 82.f), FLinearColor(0.05f, 0.15f, 0.05f), FRotator::ZeroRotator, false);
			TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(0.f, 0.f, 24.f), FVector(28.f, 44.f, 64.f), FLinearColor(0.45f, 0.05f, 0.04f), FRotator::ZeroRotator, false);
			TTShapes::AddPart(this, Root, TTShapes::Sphere(), FVector(2.f, 0.f, 70.f), FVector(22.f, 20.f, 25.f), Skin, FRotator::ZeroRotator, false);
		}
		// Short cape.
		TTShapes::AddPart(this, Root, TTShapes::Cube(), FVector(-14.f, 0.f, 22.f), FVector(4.f, 54.f, 84.f), FLinearColor(0.02f, 0.02f, 0.02f), FRotator(-6.f, 0.f, 0.f), false);
		Torch = NewObject<USpotLightComponent>(this);
		Torch->SetupAttachment(Root);
		Torch->SetRelativeLocation(FVector(20.f, 15.f, 50.f));
		Torch->SetRelativeRotation(FRotator(-8.f, 0.f, 0.f));
		Torch->SetIntensityUnits(ELightUnits::Lumens);
		Torch->SetIntensity(900.f);
		Torch->SetOuterConeAngle(16.f);
		Torch->SetInnerConeAngle(8.f);
		Torch->SetAttenuationRadius(2200.f);
		Torch->SetLightColor(FLinearColor(0.85f, 0.9f, 1.f));
		Torch->RegisterComponent();
	}
}

void ATTStoryFigure::WalkTo(const FVector& Goal, float InSpeed)
{
	Speed = InSpeed;
	ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	if (GM && GM->GetHospital())
	{
		Mover.Build(this, GM->GetHospital(), Goal, nullptr);
	}
}

void ATTStoryFigure::StartLift()
{
	LiftTime = 0.f;
	Mover.Clear();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	SetActorEnableCollision(false);
}

bool ATTStoryFigure::CanSee(const AActor* Target, float Range) const
{
	if (!Target)
	{
		return false;
	}
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 70.f);
	const FVector To = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f) - Eye;
	if (To.Size() > Range || FVector::DotProduct(To.GetSafeNormal(), GetActorForwardVector()) < 0.35f)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Target);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + To, ECC_Visibility, Params);
}

void ATTStoryFigure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (LiftTime >= 0.f)
	{
		LiftTime += DeltaSeconds;
		// Yanked up into the ceiling, then gone.
		AddActorWorldOffset(FVector(0.f, 0.f, FMath::Lerp(150.f, 900.f, FMath::Clamp(LiftTime, 0.f, 1.f)) * DeltaSeconds));
		if (LiftTime > 1.2f)
		{
			SetActorHiddenInGame(true);
		}
		return;
	}
	ATTGameMode* GM = GetWorld()->GetAuthGameMode<ATTGameMode>();
	if (Mover.HasPath() && GM && GM->GetHospital())
	{
		Mover.Follow(this, GM->GetHospital(), DeltaSeconds, Speed);
		StepTimer -= DeltaSeconds;
		if (Kind == ETTFigure::Robin && StepTimer <= 0.f && GetVelocity().Size2D() > 50.f)
		{
			StepTimer = 0.28f;
			TTAudio::PlayAt(this, ETTSfx::LightStep, GetActorLocation() - FVector(0.f, 0.f, 90.f), 0.8f);
		}
	}
}

// ===========================================================================
// Director

ATTNarrative::ATTNarrative()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	State = MakeShared<FTTNarrativeState>();
}

ATTGameMode* ATTNarrative::GM() const { return GetWorld()->GetAuthGameMode<ATTGameMode>(); }
ATTHospital* ATTNarrative::Hospital() const { return GM() ? GM()->GetHospital() : nullptr; }
ATTPlayerCharacter* ATTNarrative::Joe() const { return GM() ? GM()->GetPlayer() : nullptr; }

ATTCompanion* ATTNarrative::Companion(ETTCompanionId Id) const
{
	if (!GM())
	{
		return nullptr;
	}
	for (ATTCompanion* C : GM()->GetCompanions())
	{
		if (C && C->GetId() == Id)
		{
			return C;
		}
	}
	return nullptr;
}

FString ATTNarrative::CompanionKey(ETTCompanionId Id) const
{
	switch (Id)
	{
	case ETTCompanionId::Harley: return TEXT("harley");
	case ETTCompanionId::Bartholomeu: return TEXT("bartholomeu");
	case ETTCompanionId::Jack: return TEXT("jack");
	}
	return FString();
}

bool ATTNarrative::HasSeen(const FString& Id) const { return State->Seen.Contains(Id); }
bool ATTNarrative::HasFact(const FString& Fact) const { return State->Facts.Contains(Fact); }
void ATTNarrative::SetFact(const FString& Fact) { State->Facts.Add(Fact); }
void ATTNarrative::DebugSetFact(const FString& Fact) { SetFact(Fact); }

void ATTNarrative::MarkSeen(const FString& Scene)
{
	if (!State->Seen.Contains(Scene))
	{
		State->Seen.Add(Scene);
		UE_LOG(LogTemp, Log, TEXT("[TT] Narrative: %s (act %d, Joe at %s)"), *Scene, Act, *JoeNode);
	}
}

void ATTNarrative::FillFacts(FTTNarrativeFacts& Out) const
{
	tn::Facts& F = Out.F;
	F.difficulty = static_cast<tn::Difficulty>(static_cast<int32>(Difficulty));
	F.joeNode = Std(State->NodeOverride.IsEmpty() ? JoeNode : State->NodeOverride);
	for (const FString& S : State->Seen) F.seen.insert(Std(S));
	for (const FString& S : State->Facts) F.facts.insert(Std(S));
	ATTHospital* H = Hospital();
	for (ETTCompanionId Id : { ETTCompanionId::Harley, ETTCompanionId::Bartholomeu, ETTCompanionId::Jack })
	{
		if (const ATTCompanion* C = Companion(Id))
		{
			F.companions[Std(CompanionKey(Id))] = tn::Actor{ H ? Std(H->GetZoneAt(C->GetActorLocation())) : std::string(), C->IsVisibleTarget() };
		}
	}
	const ATTHunter* Hunter = GM() ? GM()->GetHunter() : nullptr;
	const ETTHunterState HS = Hunter ? Hunter->GetState() : ETTHunterState::Dormant;
	F.hunterHasEvidence = HS == ETTHunterState::Pursue || HS == ETTHunterState::Search || HS == ETTHunterState::Investigate;
	F.hunterCanReach = HS != ETTHunterState::Dormant && HS != ETTHunterState::Stunned;
	// The hospital always keeps a second staircase, the shafts or the elevator (M05 blockout).
	F.alternativeReachable = true;
	const int32* Visits = State->VisitCount.Find(JoeNode);
	F.areaPreviouslyVisited = Visits && *Visits >= 2;
	F.routeFailure = Std(State->RouteFailure);
}

bool ATTNarrative::Eligible(const FString& Scene) const
{
	FTTNarrativeFacts Facts;
	FillFacts(Facts);
	try
	{
		return tn::Eligible(Std(Scene), Facts.F);
	}
	catch (...)
	{
		return false;
	}
}

bool ATTNarrative::JoeHears(const AActor* Source, float Range) const
{
	const ATTPlayerCharacter* P = Joe();
	if (!Source || !P)
	{
		return true;
	}
	if (FVector::Dist(Source->GetActorLocation(), P->GetActorLocation()) > Range)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Source);
	Params.AddIgnoredActor(P);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Source->GetActorLocation() + FVector(0.f, 0.f, 60.f), P->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECC_Visibility, Params);
	// Through a wall you still hear someone close; farther away only in the open.
	return !bBlocked || FVector::Dist(Source->GetActorLocation(), P->GetActorLocation()) < Range * 0.5f;
}

// ---------------------------------------------------------------------------
// Dialogue

void ATTNarrative::Queue(const FString& Speaker, const FString& Text, float Duration, AActor* Source)
{
	FTTLine Line;
	Line.Speaker = Speaker;
	Line.Text = Text;
	Line.Duration = Duration;
	Line.Source = Source;
	Line.bAlways = Stage != EStage::Free;
	Lines.Add(Line);
}

void ATTNarrative::QueueSfx(ETTSfx Sfx, AActor* Source)
{
	FTTLine Line;
	Line.Duration = 0.f;
	Line.Source = Source;
	Line.Sfx = Sfx;
	Line.bSfx = true;
	Lines.Add(Line);
}

void ATTNarrative::UpdateLines(float DeltaSeconds)
{
	CurrentTime += DeltaSeconds;
	if (CurrentTime < CurrentDuration || Lines.Num() == 0)
	{
		return;
	}
	while (Lines.Num() > 0)
	{
		FTTLine Line = Lines[0];
		Lines.RemoveAt(0);
		const bool bHasSource = Line.Source.IsValid();
		if (Line.bSfx)
		{
			if (bHasSource)
			{
				TTAudio::PlayAt(this, Line.Sfx, Line.Source->GetActorLocation() + FVector(0.f, 0.f, 70.f), 1.f);
			}
			continue;
		}
		// Joe only hears what is said near him; the scene goes on without him.
		if (bHasSource && !Line.bAlways && !JoeHears(Line.Source.Get(), 1600.f))
		{
			continue;
		}
		CurrentSpeaker = Line.Speaker;
		CurrentText = Line.Text;
		CurrentDuration = Line.Duration;
		CurrentTime = 0.f;
		UE_LOG(LogTemp, Log, TEXT("[TT] Line %s: %s"), *Line.Speaker, *Line.Text);
		return;
	}
	CurrentText.Reset();
	CurrentSpeaker.Reset();
}

bool ATTNarrative::GetCurrentLine(FString& OutSpeaker, FString& OutText, float& OutAlpha) const
{
	if (CurrentText.IsEmpty() || CurrentTime > CurrentDuration + 0.4f)
	{
		return false;
	}
	OutSpeaker = CurrentSpeaker;
	OutText = CurrentText;
	OutAlpha = FMath::Clamp(FMath::Min(CurrentTime / 0.2f, (CurrentDuration + 0.4f - CurrentTime) / 0.4f), 0.f, 1.f);
	return true;
}

bool ATTNarrative::GetChapterCard(FString& OutTitle, float& OutAlpha) const
{
	if (ChapterTitle.IsEmpty() || ChapterTime > 6.f)
	{
		return false;
	}
	OutTitle = ChapterTitle;
	OutAlpha = FMath::Clamp(FMath::Min(ChapterTime / 1.2f, (6.f - ChapterTime) / 1.5f), 0.f, 1.f);
	return true;
}

// ---------------------------------------------------------------------------
// Documents

void ATTNarrative::OpenDocument(const FString& Id)
{
	const FDoc* Doc = FindDoc(Id);
	if (!Doc)
	{
		return;
	}
	ReadingId = Id;
	TTAudio::Play2D(this, ETTSfx::Pickup, 0.4f);
	GM()->AddNote(Doc->Note);
	SetFact(Id.ToLower() + TEXT("_read"));
	if (Id == TEXT("DOC07"))
	{
		SetFact(TEXT("isolation_known"));
	}
	if (Id == TEXT("DOC16"))
	{
		SetFact(TEXT("joe_verified_batman_record"));
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] Document %s read"), *Id);
}

void ATTNarrative::CloseDocument()
{
	ReadingId.Reset();
}

FString ATTNarrative::GetReadingTitle() const
{
	const FDoc* Doc = FindDoc(ReadingId);
	return Doc ? Doc->Title : FString();
}

FString ATTNarrative::GetReadingBody() const
{
	const FDoc* Doc = FindDoc(ReadingId);
	return Doc ? Doc->Body : FString();
}

// ---------------------------------------------------------------------------
// Setup

void ATTNarrative::Begin(EIntro Mode)
{
	Difficulty = GM()->GetDifficulty();
	SpawnStoryDevices();
	SpawnBatSignal();
	ATTPlayerCharacter* P = Joe();
	ATTHospital* H = Hospital();
	if (!P || !H)
	{
		Stage = EStage::Free;
		return;
	}
	if (Mode != EIntro::Play)
	{
		MarkSeen(TEXT("N01"));
		MarkSeen(TEXT("N02"));
		MarkSeen(TEXT("N03"));
		Stage = EStage::Free;
		ShowBatSignal(false);
		H->SetEmergency(true);
		if (Mode == EIntro::SkipScattered)
		{
			Scatter(true);
		}
		return;
	}

	// N01: everyone together in the ward where the night begins.
	Stage = EStage::Siege;
	StageTime = 0.f;
	const FIntPoint Start = H->GetPlayerStartCell();
	TArray<FIntPoint> Around;
	for (const FIntPoint& Cell : H->GetWalkableCellsNear(Start, 3))
	{
		if (Cell != Start && H->GetZoneOfCell(Cell) == H->GetZoneOfCell(Start))
		{
			Around.Add(Cell);
		}
	}
	int32 Slot = 0;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (ETTCompanionId Id : { ETTCompanionId::Harley, ETTCompanionId::Bartholomeu, ETTCompanionId::Jack })
	{
		if (ATTCompanion* C = Companion(Id))
		{
			if (Around.IsValidIndex(Slot))
			{
				C->SetActorLocation(H->CellToWorld(Around[Slot++], 94.f), false, nullptr, ETeleportType::TeleportPhysics);
			}
			C->ScriptFace(P->GetActorLocation());
		}
	}
	const FVector ThomasLoc = Around.IsValidIndex(Slot) ? H->CellToWorld(Around[Slot], 96.f) : P->GetActorLocation() + FVector(150.f, 0.f, 0.f);
	Thomas = GetWorld()->SpawnActor<ATTStoryFigure>(ATTStoryFigure::StaticClass(), ThomasLoc, FRotator::ZeroRotator, Params);
	if (Thomas)
	{
		Thomas->Setup(ETTFigure::Thomas);
		Thomas->SpawnDefaultController();
	}
	MarkSeen(TEXT("N01"));
	ATTCompanion* Harley = Companion(ETTCompanionId::Harley);
	ATTCompanion* Barth = Companion(ETTCompanionId::Bartholomeu);
	ATTCompanion* Jack = Companion(ETTCompanionId::Jack);
	Queue(JoeName, TEXT("A polícia não vai subir até aqui. Temos alguns minutos."), 3.5f);
	Queue(TEXT("Bartholomeu"), TEXT("Dois caminhos pra descer. Achei que hospital tinha um."), 3.5f, Barth);
	Queue(TEXT("Jack"), TEXT("E um poço de serviço. Esse prédio tem mais de uma vertical."), 3.5f, Jack);
	Queue(TEXT("Thomas"), TEXT("Merda. Bati o relógio na porta. Mais um risco."), 3.f, Thomas);
	Queue(TEXT("Harley"), TEXT("Você deixou isso cair."), 2.5f, Harley);
	Queue(JoeName, TEXT("Guarda. A gente conversa lá em cima."), 2.8f);
	Queue(TEXT("Harley"), TEXT("Você sempre promete isso."), 3.f, Harley);
	GM()->SetObjective(TEXT("Vá com o grupo até a janela de observação, na ala leste do 4º.   [Enter] pular introdução"));
}

void ATTNarrative::SpawnStoryDevices()
{
	ATTHospital* H = Hospital();
	if (!H)
	{
		return;
	}
	const bool bTruth = tn::ProfileFor(static_cast<tn::Difficulty>(static_cast<int32>(Difficulty))).thomasConfirmed;
	const bool bPunitive = Difficulty == ETTDifficulty::Punitive;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const ATTHospital::FStoryCell& Story : H->GetStoryCells())
	{
		ETTStation Kind = ETTStation::Document;
		switch (Story.Char)
		{
		case TEXT('d'):
			// DOC16 does not exist in Easy/Normal: no censored copy, simply not there.
			if (Story.DocId == TEXT("DOC16") && !bTruth)
			{
				continue;
			}
			break;
		case TEXT('o'):
			if (!bTruth)
			{
				continue;
			}
			Kind = ETTStation::ThomasBody;
			break;
		case TEXT('e'): if (!bPunitive) continue; Kind = ETTStation::Explosives; break;
		case TEXT('k'): if (!bPunitive) continue; Kind = ETTStation::Charge; break;
		case TEXT('j'): if (!bPunitive) continue; Kind = ETTStation::Detonator; break;
		default: continue;
		}
		const FRotator Rot(0.f, H->FacingYawForDevice(Story.Cell), 0.f);
		if (ATTStation* Station = GetWorld()->SpawnActor<ATTStation>(ATTStation::StaticClass(), H->CellToWorld(Story.Cell), Rot, Params))
		{
			Station->Setup(Kind, Story.DocId.IsEmpty() ? FString::Printf(TEXT("%c_%d_%d"), Story.Char, Story.Cell.X, Story.Cell.Y) : Story.DocId);
		}
	}
}

void ATTNarrative::SpawnBatSignal()
{
	ATTHospital* H = Hospital();
	if (!H || SignalRoot)
	{
		return;
	}
	// East of the 4th floor observation window, ~10 degrees above the eye line, over low buildings.
	const int32 F4 = FMath::Max(0, H->FloorById(TEXT("F4")));
	const FVector Window = H->CellToWorld(H->MakeCell(F4, 23, 7), 0.f);
	const FVector Signal = Window + FVector(9000.f, 700.f, 1350.f);
	SignalRoot = NewObject<USceneComponent>(this, TEXT("SignalRoot"));
	SignalRoot->SetupAttachment(RootComponent);
	SignalRoot->RegisterComponent();
	SignalRoot->SetWorldLocation(Signal);
	SignalRoot->SetWorldRotation(FRotator(0.f, (Window - Signal).Rotation().Yaw, 0.f));
	// Local +X faces the hospital.
	if (UStaticMeshComponent* Disc = TTShapes::AddPart(this, SignalRoot, TTShapes::Cylinder(), FVector::ZeroVector, FVector(950.f, 950.f, 4.f), FLinearColor::Black, FRotator(90.f, 0.f, 0.f), false))
	{
		Disc->SetMaterial(0, TTShapes::MakeEmissive(Disc, FLinearColor(1.f, 0.85f, 0.45f), 5.f));
		Disc->SetCastShadow(false);
	}
	const FLinearColor Ink(0.01f, 0.01f, 0.01f);
	auto Wing = [this, &Ink](const FVector& Pos, const FVector& Size, float Roll)
	{
		if (UStaticMeshComponent* Part = TTShapes::AddPart(this, SignalRoot, TTShapes::Cube(), Pos, Size, Ink, FRotator(0.f, 0.f, Roll), false))
		{
			Part->SetCastShadow(false);
		}
	};
	Wing(FVector(20.f, 0.f, 0.f), FVector(4.f, 110.f, 170.f), 0.f);
	Wing(FVector(20.f, -195.f, 20.f), FVector(4.f, 290.f, 80.f), -12.f);
	Wing(FVector(20.f, 195.f, 20.f), FVector(4.f, 290.f, 80.f), 12.f);
	Wing(FVector(20.f, -40.f, 105.f), FVector(4.f, 20.f, 45.f), -15.f);
	Wing(FVector(20.f, 40.f, 105.f), FVector(4.f, 20.f, 45.f), 15.f);
	// The beam, from a rooftop down in the city up to the clouds.
	const FVector Base = Signal + FVector(2500.f, -3200.f, -2550.f);
	const FVector Dir = Signal - Base;
	if (UStaticMeshComponent* Beam = TTShapes::AddPart(this, SignalRoot, TTShapes::Cylinder(), FVector::ZeroVector, FVector(260.f, 260.f, Dir.Size()), FLinearColor::Black, FRotator::ZeroRotator, false))
	{
		Beam->SetWorldLocationAndRotation((Base + Signal) * 0.5f, FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator());
		Beam->SetMaterial(0, TTShapes::MakeEmissive(Beam, FLinearColor(0.9f, 0.8f, 0.5f), 0.2f));
		Beam->SetCastShadow(false);
	}
	SignalRoot->SetVisibility(false, true);
}

void ATTNarrative::ShowBatSignal(bool bFlash)
{
	if (SignalRoot)
	{
		SignalRoot->SetVisibility(true, true);
	}
	if (bFlash)
	{
		TTAudio::Play2D(this, ETTSfx::Hiss, 0.3f);
	}
}

void ATTNarrative::Scatter(bool bTeleport)
{
	ATTHospital* H = Hospital();
	if (!H)
	{
		return;
	}
	// Where they run after N03 (anchors of N07, N08 and N15).
	struct FDest { ETTCompanionId Id; const TCHAR* Floor; int32 X; int32 Y; };
	const FDest Dests[] = {
		{ ETTCompanionId::Harley, TEXT("F3"), 14, 4 },
		{ ETTCompanionId::Jack, TEXT("F3"), 8, 4 },
		{ ETTCompanionId::Bartholomeu, TEXT("B1"), 15, 9 },
	};
	for (const FDest& D : Dests)
	{
		ATTCompanion* C = Companion(D.Id);
		const int32 F = H->FloorById(D.Floor);
		if (!C || F < 0)
		{
			continue;
		}
		const FVector Goal = H->CellToWorld(H->MakeCell(F, D.X, D.Y), 94.f);
		if (bTeleport)
		{
			C->SetActorLocation(Goal, false, nullptr, ETeleportType::TeleportPhysics);
			C->Activate();
		}
		else
		{
			C->ScriptMoveTo(Goal, true, true);
		}
		if (D.Id == ETTCompanionId::Jack && Difficulty == ETTDifficulty::Punitive)
		{
			// J1 setup: when Joe sees him again, he is not dressed the same.
			C->SetCostumeStage(1);
		}
	}
}

void ATTNarrative::SkipIntro()
{
	if (Stage == EStage::Free)
	{
		return;
	}
	Lines.Reset();
	CurrentText.Reset();
	if (Thomas)
	{
		Thomas->Destroy();
		Thomas = nullptr;
	}
	MarkSeen(TEXT("N01"));
	MarkSeen(TEXT("N02"));
	MarkSeen(TEXT("N03"));
	ShowBatSignal(false);
	Scatter(true);
	if (ATTHospital* H = Hospital())
	{
		H->SetEmergency(true);
	}
	Stage = EStage::Free;
	HunterWake = 8.f;
	GM()->ShowMessage(TEXT("Thomas sumiu. Os outros correram. Você está sozinho."), 5.f);
	GM()->SetObjective(TEXT("Saia do hospital. Algo está caçando você. [Tab] anotações"));
}

// ---------------------------------------------------------------------------
// Opening N01-N03

void ATTNarrative::UpdateIntro(float DeltaSeconds)
{
	ATTHospital* H = Hospital();
	ATTPlayerCharacter* P = Joe();
	if (!H || !P)
	{
		return;
	}
	StageTime += DeltaSeconds;
	ATTCompanion* Harley = Companion(ETTCompanionId::Harley);
	ATTCompanion* Barth = Companion(ETTCompanionId::Bartholomeu);
	ATTCompanion* Jack = Companion(ETTCompanionId::Jack);
	const int32 F4 = FMath::Max(0, H->FloorById(TEXT("F4")));
	auto At = [H, F4](int32 X, int32 Y) { return H->CellToWorld(H->MakeCell(F4, X, Y), 94.f); };

	switch (Stage)
	{
	case EStage::Siege:
		if (!IsTalking() && CurrentTime > CurrentDuration)
		{
			// The group heads for the observation windows on the east side.
			if (Harley) Harley->ScriptMoveTo(At(21, 8), false, false);
			if (Barth) Barth->ScriptMoveTo(At(20, 9), false, false);
			if (Jack) Jack->ScriptMoveTo(At(22, 9), false, false);
			if (Thomas) Thomas->WalkTo(At(22, 7), 230.f);
			Stage = EStage::ToWindow;
			StageTime = 0.f;
		}
		break;

	case EStage::ToWindow:
		if (Eligible(TEXT("N02")) || StageTime > 75.f)
		{
			const bool bAtWindow = JoeNode == TEXT("F4_OBSERVATION");
			ShowBatSignal(!bAtWindow);
			MarkSeen(TEXT("N02"));
			if (bAtWindow)
			{
				Queue(TEXT("Harley"), TEXT("Joe... olha o céu."), 2.5f, Harley);
				Queue(TEXT("Thomas"), TEXT("É só um morcego fantasiado. Relaxa."), 3.f, Thomas);
				Queue(TEXT("Harley"), TEXT("Se ele tá na rua, a gente não devia ficar na janela."), 3.5f, Harley);
				Queue(JoeName, TEXT("Sai daí, Thomas."), 2.f);
				Queue(TEXT("Jack"), TEXT("Ele não precisa estar na rua."), 3.f, Jack);
			}
			else
			{
				// Joe never went to the window: a glare crosses the ward instead.
				GM()->ShowMessage(TEXT("Um clarão amarelo atravessa as janelas do lado leste."), 4.f);
				Queue(TEXT("Harley"), TEXT("Vocês viram isso? Lá fora."), 2.5f, Harley);
				Queue(TEXT("Jack"), TEXT("Ele não precisa estar na rua."), 3.f, Jack);
			}
			Stage = EStage::Signal;
			StageTime = 0.f;
		}
		break;

	case EStage::Signal:
		if (!IsTalking() && CurrentTime > CurrentDuration + 1.f)
		{
			// Back towards the ward, along the east corridor.
			if (Harley) Harley->ScriptMoveTo(At(12, 7), false, false);
			if (Barth) Barth->ScriptMoveTo(At(14, 7), false, false);
			if (Jack) Jack->ScriptMoveTo(At(11, 6), false, false);
			if (Thomas) Thomas->WalkTo(At(16, 7), 220.f);
			GM()->SetObjective(TEXT("Volte com o grupo pelo corredor."));
			Stage = EStage::Return;
			StageTime = 0.f;
		}
		break;

	case EStage::Return:
		if ((StageTime > 4.f && Eligible(TEXT("N03"))) || (StageTime > 30.f && Eligible(TEXT("N03"))))
		{
			// N03: the premise. Lights die, a shape from above, Thomas is gone.
			MarkSeen(TEXT("N03"));
			H->SetLampsNear(P->GetActorLocation(), 2500.f, false);
			H->SetEmergency(true);
			TTAudio::Play2D(this, ETTSfx::Sting, 1.f);
			if (Thomas)
			{
				P->LookAtPoint(Thomas->GetActorLocation() + FVector(0.f, 0.f, 60.f));
				Thomas->StartLift();
				TTAudio::PlayAt(this, ETTSfx::Punch, Thomas->GetActorLocation(), 1.f);
			}
			Queue(TEXT("Thomas"), TEXT("Que porra foi essa—"), 1.4f, Thomas);
			Queue(JoeName, TEXT("Thomas?!"), 1.8f);
			Queue(TEXT("Harley"), TEXT("Joe, olha pra mim. A gente precisa sair daqui."), 3.f, Harley);
			Queue(JoeName, TEXT("Não foi a polícia."), 2.5f);
			Stage = EStage::Rupture;
			StageTime = 0.f;
		}
		else if (StageTime > 25.f && StageTime - DeltaSeconds <= 25.f)
		{
			Queue(TEXT("Harley"), TEXT("Joe, vem. Não fica aí sozinho."), 3.f, Harley);
		}
		break;

	case EStage::Rupture:
		if (StageTime > 2.2f && Thomas)
		{
			Thomas->Destroy();
			Thomas = nullptr;
			Scatter(false);
			TTAudio::Play2D(this, ETTSfx::Breath, 0.8f);
		}
		if (StageTime > 9.f)
		{
			H->SetLampsNear(P->GetActorLocation(), 2500.f, true);
			Stage = EStage::Free;
			HunterWake = 12.f;
			GM()->ShowMessage(TEXT("Thomas sumiu. Os outros correram. Você está sozinho."), 5.f);
			GM()->SetObjective(TEXT("Saia do hospital. Algo está caçando você. [Tab] anotações"));
		}
		break;

	case EStage::Free:
		break;
	}
}

// ---------------------------------------------------------------------------
// Beats tied to places and world facts (N04-N34)

void ATTNarrative::PlayBeat(const FString& Scene)
{
	MarkSeen(Scene);
	const bool bPunitive = Difficulty == ETTDifficulty::Punitive;
	if (Scene == TEXT("N04"))
	{
		Queue(JoeName, TEXT("(baixo) Devagar. Agachado. Escuta antes de andar."), 3.5f);
		GM()->ShowMessage(TEXT("Agachado (C) você faz menos barulho. Armários (E) escondem. Ele ouve, ele vê, ele lembra."), 6.f);
	}
	else if (Scene == TEXT("N05"))
	{
		SetFact(TEXT("thomas_mentioned"));
		Queue(JoeName, TEXT("Lençol rasgado, um sapato... Ele passou por aqui?"), 3.5f);
	}
	else if (Scene == TEXT("N06"))
	{
		Queue(JoeName, TEXT("(baixo) Cortina, leito, vidro. Dá pra ver sem ser visto. Ele também."), 3.5f);
	}
	else if (Scene == TEXT("N09"))
	{
		Queue(JoeName, TEXT("Câmeras. Se ainda funcionam, eu vejo ele antes de ele me ver."), 3.5f);
	}
	else if (Scene == TEXT("N10"))
	{
		Queue(JoeName, TEXT("(baixo) Esse reflexo já passou duas vezes. Isso é gravação, não é agora."), 4.f);
	}
	else if (Scene == TEXT("N11"))
	{
		Queue(JoeName, TEXT("(baixo) Pensa, Joe. Pensa."), 2.5f);
	}
	else if (Scene == TEXT("N12"))
	{
		Queue(JoeName, TEXT("Prateleira limpa demais. Alguém andou cuidando disso aqui."), 3.5f);
	}
	else if (Scene == TEXT("N13"))
	{
		Queue(JoeName, TEXT("Tem carro lá embaixo. Claro que a chave ficou no andar de cima."), 3.5f);
	}
	else if (Scene == TEXT("N14"))
	{
		Queue(JoeName, TEXT("Três pedaços de um número. Turno, arquivo, manutenção. Alguém aqui tinha medo de esquecer."), 4.5f);
	}
	else if (Scene == TEXT("N16"))
	{
		Queue(JoeName, TEXT("Esse ronco... é o gerador. Se eu ligar isso, o prédio inteiro fica sabendo."), 4.f);
	}
	else if (Scene == TEXT("N17"))
	{
		Queue(JoeName, TEXT("O mesmo duto em todos os andares. Ele não passa aqui. Eu passo."), 3.5f);
	}
	else if (Scene == TEXT("N18"))
	{
		Queue(JoeName, JoeNode == TEXT("F2_SUPPLY") ? TEXT("Ferramenta. Pesada, barulhenta... serve pra uma janela pregada.")
			: TEXT("Dá pro prédio do lado. Pregada. Sem ferramenta não sai."), 3.5f);
	}
	else if (Scene == TEXT("N20"))
	{
		Queue(JoeName, TEXT("Corrente de ar. Água correndo. Isso vai dar em algum lugar."), 3.5f);
	}
	else if (Scene == TEXT("N21"))
	{
		Queue(JoeName, TEXT("(baixo) Ele estava esperando. Ele sabe por onde eu ando."), 3.5f);
	}
	else if (Scene == TEXT("N22"))
	{
		Queue(JoeName, TEXT("Metade do prédio acordou. A outra metade vai vir ver o que foi."), 3.5f);
	}
	else if (Scene == TEXT("N27"))
	{
		Queue(JoeName, TEXT("Uma porta pesada no fundo. Isolamento. Ninguém lembra que isso existe."), 3.5f);
	}
	else if (Scene == TEXT("N34"))
	{
		SetFact(TEXT("thomas_known"));
		Queue(JoeName, TEXT("É o Thomas."), 2.5f);
		Queue(FString(), TEXT("[O relógio riscado. O horário da fita. A mesma marca.]"), 3.5f);
		Queue(JoeName, TEXT("Ele não levou o Thomas daqui. Ele o matou."), 4.f);
	}
	(void)bPunitive;
}

void ATTNarrative::UpdateWorld()
{
	ATTHospital* H = Hospital();
	ATTPlayerCharacter* P = Joe();
	ATTGameMode* G = GM();
	if (!H || !P || !G)
	{
		return;
	}
	const FString Node = H->GetZoneAt(P->GetActorLocation());
	if (Node != JoeNode)
	{
		JoeNode = Node;
		State->VisitCount.FindOrAdd(JoeNode)++;
	}
	if (Stage != EStage::Free || IsTalking())
	{
		return;
	}

	// World facts the contract reads (all observed through the run flags).
	if (G->HasRunFlag(TEXT("power_restored")) || G->HasRunFlag(TEXT("interference_clear")))
	{
		SetFact(TEXT("circuit_changed"));
	}
	const ATTHunter* Hunter = G->GetHunter();
	const ETTHunterState HS = Hunter ? Hunter->GetState() : ETTHunterState::Dormant;
	if (HS == ETTHunterState::Ambush || HS == ETTHunterState::SetTrap)
	{
		SetFact(TEXT("hunter_observed_habit"));
	}
	if (HS == ETTHunterState::PursueCompanion || HS == ETTHunterState::Interrogate)
	{
		CompanionRiskAt = GetWorld()->GetTimeSeconds();
	}

	// One beat per poll; each plays once (scene_seen).
	struct FBeat { const TCHAR* Scene; bool bExtra; };
	const bool bViewing = P->IsViewingCameras();
	const FBeat Beats[] = {
		{ TEXT("N04"), true },
		{ TEXT("N05"), true },
		{ TEXT("N06"), true },
		{ TEXT("N09"), true },
		{ TEXT("N10"), bViewing },
		{ TEXT("N11"), HS == ETTHunterState::Pursue },
		{ TEXT("N12"), true },
		{ TEXT("N13"), G->HasRunFlag(TEXT("key_known")) },
		{ TEXT("N14"), G->HasRunFlag(TEXT("code_b")) || G->HasRunFlag(TEXT("safe_known")) },
		{ TEXT("N16"), true },
		{ TEXT("N17"), true },
		{ TEXT("N18"), true },
		{ TEXT("N20"), G->HasRunFlag(TEXT("tunnel_found")) },
		{ TEXT("N21"), true },
		{ TEXT("N22"), G->HasRunFlag(TEXT("power_restored")) },
		{ TEXT("N27"), true },
		{ TEXT("N34"), true },
	};
	for (const FBeat& Beat : Beats)
	{
		if (Beat.bExtra && !HasSeen(Beat.Scene) && Eligible(Beat.Scene))
		{
			PlayBeat(Beat.Scene);
			return;
		}
	}

	// N26: a prepared route broke, observably. Once per route.
	static const TCHAR* Routes[] = { TEXT("garage"), TEXT("roof"), TEXT("tunnel"), TEXT("window") };
	for (int32 R = 0; R < 4; ++R)
	{
		if (!G->IsRouteFailed(R) || HasSeen(FString::Printf(TEXT("N26:%s"), Routes[R])))
		{
			continue;
		}
		SetFact(TEXT("route_failure_observed"));
		SetFact(FString::Printf(TEXT("route_failed:%s"), Routes[R]));
		State->RouteFailure = Routes[R];
		const bool bOk = Eligible(TEXT("N26"));
		State->RouteFailure.Reset();
		if (bOk)
		{
			MarkSeen(FString::Printf(TEXT("N26:%s"), Routes[R]));
			MarkSeen(TEXT("N26"));
			switch (R)
			{
			case 1: Queue(JoeName, TEXT("O barulho do rotor indo embora... Foi. Ele não esperou."), 4.f); break;
			case 3: Queue(JoeName, TEXT("Ele achou a janela. Pregou de novo. Outro caminho, Joe."), 4.f); break;
			default: Queue(JoeName, TEXT("Esse caminho morreu. Pensa em outro."), 3.f); break;
			}
			G->AddNote(R == 1 ? TEXT("O helicóptero foi embora.") : R == 3 ? TEXT("A janela do 3º foi lacrada.") : TEXT("Uma rota caiu."));
			return;
		}
	}

	// Radio intercepts once the radio has power (AUD04, AUD05; Robin's foreshadowing in Punitive).
	if (G->HasRunFlag(TEXT("batteries_installed")) && !HasFact(TEXT("aud04")) && JoeNode == TEXT("F1_RADIO"))
	{
		SetFact(TEXT("aud04"));
		Queue(TEXT("Rádio (polícia)"), TEXT("...quarteirão isolado. Ninguém cruza o cordão até o dia clarear..."), 4.f);
		Queue(TEXT("Rádio (voz grave)"), TEXT("...mantenha-os fora."), 2.5f);
		if (Difficulty == ETTDifficulty::Punitive)
		{
			Queue(TEXT("Rádio (outra voz)"), TEXT("...Robin, cubra a linha pluvial. Entendido. Descendo..."), 4.f);
		}
		else
		{
			Queue(TEXT("Rádio (voz idosa)"), TEXT("...o senhor está aí dentro há duas horas. Por favor, tome cuidado..."), 4.f);
		}
	}
}

void ATTNarrative::OnRadioContact()
{
	MarkSeen(TEXT("N19"));
	Queue(TEXT("Rádio"), TEXT("Joe? ...Repete o ponto de retirada. Não vou entrar às cegas."), 4.f);
	Queue(JoeName, TEXT("Telhado do hospital. Confirma que vem."), 3.f);
	Queue(TEXT("Rádio"), TEXT("Tá. Fica vivo. Telhado. Cinco minutos, nem um a mais."), 4.f);
}

// ---------------------------------------------------------------------------
// Acts: narrative milestones, never minutes

void ATTNarrative::UpdateActs()
{
	ATTGameMode* G = GM();
	ATTPlayerCharacter* P = Joe();
	ATTHospital* H = Hospital();
	if (!G || !P || !H)
	{
		return;
	}
	int32 Target = 1;
	if (HasSeen(TEXT("N03")))
	{
		Target = 2;
	}
	const int32 F2 = H->FloorById(TEXT("F2"));
	const bool bLower = P->GetFloor() <= F2 && P->GetFloor() >= 0;
	int32 Prepared = 0;
	for (const TCHAR* Flag : { TEXT("safe_known"), TEXT("radio_found"), TEXT("tunnel_found"), TEXT("window_found"), TEXT("power_restored"), TEXT("key_known") })
	{
		Prepared += G->HasRunFlag(Flag) ? 1 : 0;
	}
	if (Target >= 2 && (bLower || Prepared >= 1))
	{
		Target = 3;
	}
	if (Target >= 3 && Prepared >= 3)
	{
		Target = 4;
	}
	if (Target >= 3 && (G->HasRunFlag(TEXT("gate_released")) || G->HasRunFlag(TEXT("helicopter_confirmed"))
		|| G->HasRunFlag(TEXT("window_unblocked")) || (G->HasRunFlag(TEXT("tunnel_found")) && P->GetFloor() == H->FloorById(TEXT("T")))))
	{
		Target = 5;
	}
	if (Target > Act)
	{
		Act = Target;
		static const TCHAR* Titles[] = { TEXT(""), TEXT("I · Entrada e ruptura"), TEXT("II · O hospital também observa"),
			TEXT("III · O hospital como ferramenta"), TEXT("IV · Pressão"), TEXT("V · A saída que a noite construiu") };
		ChapterTitle = Titles[Act];
		ChapterTime = 0.f;
		UE_LOG(LogTemp, Log, TEXT("[TT] Act %d"), Act);
	}
}

// ---------------------------------------------------------------------------
// Companions: scenes when Joe talks to them

bool ATTNarrative::TryCompanionScene(ATTCompanion* C, ATTPlayerCharacter* P)
{
	const bool bPlayed = PlayCompanionScene(C, P);
	if (bPlayed && C)
	{
		float Total = 1.f;
		for (const FTTLine& Line : Lines)
		{
			Total += Line.Duration;
		}
		C->HoldForTalk(Total);
	}
	return bPlayed;
}

bool ATTNarrative::PlayCompanionScene(ATTCompanion* C, ATTPlayerCharacter* P)
{
	if (!C || !P || Stage != EStage::Free || IsTalking())
	{
		return false;
	}
	ATTHospital* H = Hospital();
	JoeNode = H ? H->GetZoneAt(P->GetActorLocation()) : JoeNode;
	LastContact.Add(C->GetId(), JoeNode);
	const bool bPunitive = Difficulty == ETTDifficulty::Punitive;
	const bool bInjured = P->GetInjury() > 0 || P->IsBleeding();
	const FString Name = C->GetDisplayName();
	auto Clue = [this, C](const TCHAR* Id, int32 InAct)
	{
		FTTNarrativeFacts Facts;
		FillFacts(Facts);
		try
		{
			const bool bSight = FVector::Dist(C->GetActorLocation(), Joe()->GetActorLocation()) < 600.f;
			return FString(Id) == TEXT("J1") ? tn::CanWitnessJ1(Facts.F, bSight) : tn::CanWitnessLateClue(Std(Id), Facts.F, InAct, bSight);
		}
		catch (...)
		{
			return false;
		}
	};

	switch (C->GetId())
	{
	case ETTCompanionId::Harley:
		if (Act >= 4)
		{
			SetFact(TEXT("harley_under_pressure"));
		}
		if (Eligible(TEXT("N23")))
		{
			MarkSeen(TEXT("N23"));
			if (bPunitive)
			{
				Queue(Name, TEXT("Eu consigo ir. Você não precisa voltar."), 3.f, C);
				Queue(JoeName, TEXT("Preciso saber que você chegou. Me diz por onde eu te encontro."), 4.f);
			}
			else
			{
				Queue(Name, TEXT("Você ouviu quando eu pedi ajuda?"), 3.f, C);
				Queue(JoeName, TEXT("Ouvi. Estou vendo se dá pra passar."), 3.f);
				Queue(Name, TEXT("Eu perguntei de mim, Joe."), 3.f, C);
			}
			return true;
		}
		if (Eligible(TEXT("N07")))
		{
			MarkSeen(TEXT("N07"));
			if (bInjured)
			{
				Queue(Name, TEXT("Você tá sangrando."), 2.f, C);
				Queue(JoeName, TEXT("Não é nada."), 1.8f);
				Queue(Name, TEXT("Não mente pra mim. Encosta aqui, eu vejo."), 3.f, C);
				// An exchange that actually happens: she hands over what she has.
				if (P->TryAddItem(ETTItem::Bandage, 1))
				{
					Queue(Name, TEXT("Toma. Aperta firme e não tira."), 2.5f, C);
				}
			}
			else
			{
				Queue(Name, TEXT("Escutei uma porta de metal bater duas vezes. Alguém usou o corredor do outro lado."), 4.f, C);
				Queue(JoeName, TEXT("Você viu quem?"), 2.f);
				Queue(Name, TEXT("Não. Eu ouvi a porta."), 2.5f, C);
			}
			Queue(Name, TEXT("Eu também me cortei naquela sala."), 2.8f, C);
			if (bPunitive)
			{
				Queue(JoeName, TEXT("Deixa eu ver. Se esse corredor piorar, eu volto por você."), 3.5f);
				Queue(Name, TEXT("Então não me faz procurar você sozinha."), 3.f, C);
			}
			else
			{
				Queue(JoeName, TEXT("Você ainda consegue andar. Vamos."), 2.5f);
				Queue(Name, AmmoGiven.Contains(ETTCompanionId::Harley) ? TEXT("Consigo. Você me ajudou lá atrás... então não me trata como plano B agora.")
					: TEXT("Consigo. Era só isso que você precisava saber?"), 3.5f, C);
			}
			return true;
		}
		return false;

	case ETTCompanionId::Bartholomeu:
		if (Act >= 4)
		{
			SetFact(TEXT("barth_under_pressure"));
		}
		if (Eligible(TEXT("N24")))
		{
			MarkSeen(TEXT("N24"));
			Queue(Name, TEXT("Eu segurei a porta por você quando entramos."), 3.f, C);
			Queue(JoeName, TEXT("E agora?"), 1.5f);
			Queue(Name, TEXT("Agora é você que decide se deixa ela aberta."), 3.5f, C);
			return true;
		}
		if (Eligible(TEXT("N15")))
		{
			MarkSeen(TEXT("N15"));
			Queue(Name, TEXT("Tem um caminho perto das bombas. Talvez saia do outro lado."), 3.5f, C);
			Queue(JoeName, TEXT("Talvez?"), 1.5f);
			Queue(Name, TEXT("Eu vi a entrada, Joe. Não vi a saída. Se quer certeza, vem comigo."), 4.f, C);
			Queue(Name, TEXT("Se o Thomas estivesse aqui, já tinha descido."), 3.f, C);
			SetFact(TEXT("barth_told_tunnel"));
			// Jack hears it only if he is actually there.
			if (const ATTCompanion* Jack = Companion(ETTCompanionId::Jack))
			{
				if (Jack->IsVisibleTarget() && FVector::Dist(Jack->GetActorLocation(), C->GetActorLocation()) < 900.f)
				{
					SetFact(TEXT("jack_heard_thomas"));
				}
			}
			GM()->AddNote(TEXT("Bartholomeu: viu uma entrada perto das bombas (subsolo, sudeste). Não sabe onde sai."));
			return true;
		}
		return false;

	case ETTCompanionId::Jack:
	{
		// Encounter facts are about this conversation only (never persisted).
		const bool bHeard = bInjured || HasFact(TEXT("jack_heard_thomas"));
		if (HasSeen(TEXT("N08")))
		{
			SetFact(TEXT("jack_claimed_route"));
		}
		if (GetWorld()->GetTimeSeconds() - CompanionRiskAt < 120.f)
		{
			SetFact(TEXT("jack_perceived_companion_risk"));
		}
		auto ClearEncounter = [this]()
		{
			State->Facts.Remove(TEXT("jack_act3_encounter"));
			State->Facts.Remove(TEXT("jack_heard_thomas_or_injury"));
			State->Facts.Remove(TEXT("jack_final_encounter"));
			State->Facts.Remove(TEXT("jack_perceived_companion_risk"));
		};

		// J4: the reveal (Punitive, act 5, J3 and one more clue witnessed).
		SetFact(TEXT("jack_final_encounter"));
		if (!HasSeen(TEXT("J4")) && Act >= 5 && Clue(TEXT("J4"), Act))
		{
			ClearEncounter();
			MarkSeen(TEXT("J4"));
			SetFact(TEXT("J4"));
			C->SetCostumeStage(3);
			Queue(JoeName, TEXT("Quem diabos é você?"), 2.5f);
			Queue(FString(), TEXT("[Silêncio.]"), 2.f);
			Queue(Name, TEXT("Você realmente não me reconheceu?"), 3.f, C);
			Queue(Name, TEXT("Você passou a noite inteira olhando para o homem no teto."), 3.5f, C);
			Queue(Name, TEXT("Nem uma vez olhou para quem andava ao seu lado."), 3.5f, C);
			QueueSfx(ETTSfx::LaughLong, C);
			Queue(FString(), TEXT("[Ele ri. E não para.]"), 3.f, C);
			C->LeaveForGood();
			return true;
		}
		State->Facts.Remove(TEXT("jack_final_encounter"));

		if (Act >= 4 && Eligible(TEXT("N25")))
		{
			MarkSeen(TEXT("N25"));
			Queue(Name, TEXT("Tem um atalho pela prumada de serviço. Eu fui até a metade."), 3.5f, C);
			Queue(JoeName, TEXT("E a outra metade?"), 2.f);
			Queue(Name, TEXT("Aí é com você. Ou confere antes. Eu conferiria."), 3.5f, C);
			if (bPunitive && Clue(TEXT("J3"), Act))
			{
				SetFact(TEXT("J3"));
				MarkSeen(TEXT("J3"));
				C->SetCostumeStage(3);
				Queue(JoeName, TEXT("Ele pode estar preso lá."), 2.5f);
				Queue(Name, TEXT("Então alguém enfim achou um quarto reservado."), 3.f, C);
				QueueSfx(ETTSfx::LaughLong, C);
				Queue(FString(), TEXT("[Jack ri. Tarde demais. Por tempo demais.]"), 3.f, C);
				Queue(JoeName, TEXT("Isso é engraçado pra você?"), 2.5f);
			}
			ClearEncounter();
			return true;
		}

		// J2: a second encounter in act 3+, after hearing about Thomas or seeing Joe hurt.
		if (bPunitive && HasSeen(TEXT("N08")) && !HasSeen(TEXT("J2")) && Act >= 3)
		{
			SetFact(TEXT("jack_act3_encounter"));
			if (bHeard)
			{
				SetFact(TEXT("jack_heard_thomas_or_injury"));
			}
			if (Clue(TEXT("J2"), Act))
			{
				ClearEncounter();
				SetFact(TEXT("J2"));
				MarkSeen(TEXT("J2"));
				C->SetCostumeStage(2);
				Queue(Name, bInjured ? TEXT("Olha só você. Vazando. Daqui a pouco ele te acha pelo cheiro.")
					: TEXT("O Thomas sempre quis sair no jornal. Vai sair."), 3.5f, C);
				QueueSfx(ETTSfx::Laugh, C);
				Queue(FString(), TEXT("[Jack ri. A risada dura um pouco mais do que devia.]"), 3.f, C);
				return true;
			}
			ClearEncounter();
		}

		if (Eligible(TEXT("N08")))
		{
			MarkSeen(TEXT("N08"));
			if (C->GetHunterSightings() >= 2)
			{
				Queue(Name, TEXT("Na primeira vez, ele veio pelo barulho. Na segunda, pela escada."), 3.5f, C);
				Queue(JoeName, TEXT("Tá dizendo que ele sabe para onde eu vou?"), 2.5f);
				Queue(Name, TEXT("Tô dizendo que você acabou de me contar qual escada costuma usar."), 3.5f, C);
			}
			else
			{
				Queue(Name, TEXT("Se ele ouviu a porta, talvez esteja olhando o corredor errado agora."), 3.5f, C);
				Queue(JoeName, TEXT("Você viu ele?"), 1.8f);
				Queue(Name, TEXT("Vi a porta aberta. O resto é palpite."), 3.f, C);
			}
			if (bPunitive && Clue(TEXT("J1"), Act))
			{
				SetFact(TEXT("J1"));
				MarkSeen(TEXT("J1"));
				Queue(JoeName, bInjured ? TEXT("Eu tô machucado, Jack.") : TEXT("E o Thomas?"), 2.f);
				Queue(Name, bInjured ? TEXT("Tá? Imagina o Thomas.") : TEXT("O Thomas? O Thomas subiu na vida."), 2.5f, C);
				QueueSfx(ETTSfx::Laugh, C);
				Queue(FString(), TEXT("[Uma risada curta. Fora de hora.]"), 2.5f, C);
			}
			// A true observation that helps: Jack shares what he knows.
			C->ShareKnownRoute();
			return true;
		}
		ClearEncounter();
		return false;
	}
	}
	return false;
}

void ATTNarrative::OnCompanionTaken(ATTCompanion* C, bool bJoeHeard)
{
	if (!C)
	{
		return;
	}
	CompanionRiskAt = GetWorld()->GetTimeSeconds();
	if (bJoeHeard)
	{
		CaptureVerified.Add(C->GetId());
		LastContact.Add(C->GetId(), Hospital() ? Hospital()->GetZoneAt(C->GetActorLocation()) : FString());
	}
}

void ATTNarrative::OnCompanionEscaped(ATTCompanion* C, bool bJoeHeard)
{
	if (C && bJoeHeard)
	{
		EscapeVerified.Add(C->GetId());
	}
}

// ---------------------------------------------------------------------------
// Stations: documents, Thomas, Punitive explosives

FString ATTNarrative::GetStationPrompt(const ATTStation* Station) const
{
	if (!Station)
	{
		return FString();
	}
	switch (Station->GetKind())
	{
	case ETTStation::Document:
		if (Station->GetTag() == TEXT("DOC16"))
		{
			return HasFact(TEXT("doc16_read")) ? TEXT("[E] Rever a fita arquivada") : TEXT("[E] Ver a fita arquivada (CAM_F4_WEST)");
		}
		return HasFact(Station->GetTag().ToLower() + TEXT("_read")) ? TEXT("[E] Reler") : TEXT("[E] Ler");
	case ETTStation::ThomasBody:
		return HasFact(TEXT("joe_identified_thomas_body")) ? TEXT("Thomas") : TEXT("[E] Levantar o lençol");
	case ETTStation::Explosives:
		return HasFact(TEXT("explosives_taken")) ? TEXT("Caixa vazia") : TEXT("[E] Pegar os explosivos de demolição");
	case ETTStation::Charge:
		if (State->ChargesArmed.Contains(Station->GetTag())) return TEXT("Carga armada");
		return HasFact(TEXT("explosives_taken")) ? TEXT("[E] Armar uma carga no pilar (barulho)") : TEXT("Pilar estrutural marcado de vermelho");
	case ETTStation::Detonator:
		return ChargesPlanted >= 3 ? TEXT("[E] Detonar. Não tem volta.") : FString::Printf(TEXT("Detonador. Faltam %d cargas."), 3 - ChargesPlanted);
	default:
		return FString();
	}
}

void ATTNarrative::UseStation(ATTStation* Station, ATTPlayerCharacter* P)
{
	ATTGameMode* G = GM();
	if (!Station || !G)
	{
		return;
	}
	switch (Station->GetKind())
	{
	case ETTStation::Document:
		OpenDocument(Station->GetTag());
		break;

	case ETTStation::ThomasBody:
		if (!HasFact(TEXT("joe_identified_thomas_body")))
		{
			SetFact(TEXT("joe_identified_thomas_body"));
			TTAudio::Play2D(this, ETTSfx::Sting, 0.6f);
			G->ShowMessage(TEXT("Um braço sob o lençol. No pulso, um relógio riscado. O mesmo risco da porta, lá em cima."), 7.f);
			G->AddNote(TEXT("Necrotério (subsolo): o corpo tem o relógio riscado do Thomas."));
			if (!HasFact(TEXT("joe_verified_batman_record")))
			{
				Queue(JoeName, TEXT("Thomas...? Não. Pode ser de qualquer um. Tem que ter mais."), 4.f);
			}
		}
		break;

	case ETTStation::Explosives:
		if (!HasFact(TEXT("explosives_taken")))
		{
			SetFact(TEXT("explosives_taken"));
			TTAudio::Play2D(this, ETTSfx::Pickup, 0.6f);
			G->AddNote(TEXT("Explosivos de demolição: três cargas. Pilares marcados de vermelho: garagem, saguão do 1º, geradores do subsolo. Detonador: nos túneis."));
			G->ShowMessage(TEXT("Cargas de demolição, da reforma que nunca aconteceu. Três pilares estão marcados com um X vermelho."), 6.f);
			Queue(JoeName, TEXT("Isso derruba o prédio inteiro. Com ele dentro. Com todo mundo dentro."), 4.f);
		}
		break;

	case ETTStation::Charge:
		if (HasFact(TEXT("explosives_taken")) && !State->ChargesArmed.Contains(Station->GetTag()))
		{
			State->ChargesArmed.Add(Station->GetTag());
			++ChargesPlanted;
			Station->SetStatus(FLinearColor(1.f, 0.05f, 0.02f));
			TTAudio::PlayAt(this, ETTSfx::Clank, Station->GetActorLocation(), 0.8f);
			G->ReportNoise(Station->GetActorLocation(), ETTNoise::Door, P);
			G->ShowMessage(FString::Printf(TEXT("Carga armada. %d de 3."), ChargesPlanted), 3.f);
			UE_LOG(LogTemp, Log, TEXT("[TT] Charge armed (%d/3)"), ChargesPlanted);
		}
		break;

	case ETTStation::Detonator:
		if (ChargesPlanted >= 3 && !HasFact(TEXT("rare_destruction")))
		{
			SetFact(TEXT("rare_destruction"));
			TTAudio::Play2D(this, ETTSfx::Explosion, 1.f);
			TTAudio::Play2D(this, ETTSfx::Explosion, 0.8f);
			if (ATTHospital* H = Hospital())
			{
				H->SetLampsNear(P ? P->GetActorLocation() : FVector::ZeroVector, 100000.f, false);
			}
			G->ApplyRunEvent(static_cast<int32>(tr::Event::HospitalDestroyed));
			G->FinishIfEnded();
		}
		break;

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Rare endings and Robin

void ATTNarrative::UpdateConfrontationHint()
{
	if (Difficulty != ETTDifficulty::Easy || bConfrontHintSaid || Stage != EStage::Free || IsTalking())
	{
		return;
	}
	TArray<ATTCompanion*> Following;
	for (ATTCompanion* C : GM()->GetCompanions())
	{
		if (C && C->IsActive() && C->GetState() == ETTCompanionState::Following && JoeHears(C, 900.f))
		{
			Following.Add(C);
		}
	}
	if (Following.Num() >= 2)
	{
		bConfrontHintSaid = true;
		ATTCompanion* Speaker = Following[0];
		for (ATTCompanion* C : Following)
		{
			if (C->GetId() == ETTCompanionId::Bartholomeu)
			{
				Speaker = C;
			}
		}
		Queue(Speaker->GetDisplayName(), TEXT("Se ele cair, a gente pula em cima dele. Todo mundo junto. É a única chance."), 4.5f, Speaker);
	}
}

void ATTNarrative::OnHunterStunned()
{
	// GDD §59, Easy: survivors coordinate. Two trusted companions with Joe, the Hunter down.
	ATTGameMode* G = GM();
	ATTPlayerCharacter* P = Joe();
	const ATTHunter* Hunter = G ? G->GetHunter() : nullptr;
	if (Difficulty != ETTDifficulty::Easy || !G || !P || !Hunter || Stage != EStage::Free)
	{
		return;
	}
	int32 Close = 0;
	for (ATTCompanion* C : G->GetCompanions())
	{
		if (C && C->IsActive() && C->GetState() == ETTCompanionState::Following
			&& FVector::Dist(C->GetActorLocation(), Hunter->GetActorLocation()) < 1200.f)
		{
			++Close;
		}
	}
	if (Close >= 2 && FVector::Dist(P->GetActorLocation(), Hunter->GetActorLocation()) < 1500.f)
	{
		SetFact(TEXT("rare_confrontation"));
		TTAudio::Play2D(this, ETTSfx::Punch, 1.f);
		TTAudio::Play2D(this, ETTSfx::Punch, 0.8f);
		G->ApplyRunEvent(static_cast<int32>(tr::Event::CoordinatedConfrontation));
		G->FinishIfEnded();
	}
}

void ATTNarrative::UpdateRobin(float DeltaSeconds)
{
	// GDD §76: Punitive only, a special episode in the tunnels, never a second permanent Hunter.
	ATTHospital* H = Hospital();
	ATTPlayerCharacter* P = Joe();
	if (Difficulty != ETTDifficulty::Punitive || !H || !P || RobinStage >= 3)
	{
		return;
	}
	const int32 TFloor = H->FloorById(TEXT("T"));
	if (RobinStage == 0)
	{
		TunnelTime = P->GetFloor() == TFloor ? TunnelTime + DeltaSeconds : TunnelTime;
		if (TunnelTime > 12.f)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Robin = GetWorld()->SpawnActor<ATTStoryFigure>(ATTStoryFigure::StaticClass(), H->CellToWorld(H->MakeCell(TFloor, 20, 1), 94.f), FRotator::ZeroRotator, Params);
			if (Robin)
			{
				Robin->Setup(ETTFigure::Robin);
				Robin->SpawnDefaultController();
				Robin->WalkTo(P->GetActorLocation(), 300.f);
				RobinStage = 1;
				RobinTime = 0.f;
				UE_LOG(LogTemp, Log, TEXT("[TT] Robin enters the tunnels"));
			}
			else
			{
				RobinStage = 3;
			}
		}
		return;
	}
	if (!Robin)
	{
		RobinStage = 3;
		return;
	}
	RobinTime += DeltaSeconds;
	if (RobinStage == 1)
	{
		if (Robin->CanSee(P, 1500.f) && P->GetVisibility() > 0.15f)
		{
			// He does not grab Joe: he reports, and the Hunter comes down.
			Queue(TEXT("Robin"), TEXT("Ele está aqui embaixo. Galeria pluvial, ramal leste."), 3.5f, Robin);
			if (ATTHunter* Hunter = GM()->GetHunter())
			{
				Hunter->OnNoiseHeard(P->GetActorLocation(), ETTNoise::Gunshot);
			}
			SetFact(TEXT("robin_reported"));
			Robin->WalkTo(H->CellToWorld(H->MakeCell(TFloor, 20, 1), 94.f), 320.f);
			RobinStage = 2;
			RobinTime = 0.f;
		}
		else if (RobinTime > 45.f || Robin->HasArrived())
		{
			Queue(TEXT("Robin"), TEXT("Nada aqui. Subindo."), 2.5f, Robin);
			Robin->WalkTo(H->CellToWorld(H->MakeCell(TFloor, 20, 1), 94.f), 300.f);
			RobinStage = 2;
			RobinTime = 0.f;
		}
	}
	else if (RobinStage == 2 && (Robin->HasArrived() || RobinTime > 40.f))
	{
		Robin->Destroy();
		Robin = nullptr;
		RobinStage = 3;
	}
}

// ---------------------------------------------------------------------------
// Evidence and epilogues

void ATTNarrative::UpdateEvidence()
{
	ATTHospital* H = Hospital();
	ATTPlayerCharacter* P = Joe();
	if (!H || !P)
	{
		return;
	}
	for (ATTCompanion* C : GM()->GetCompanions())
	{
		if (C && C->IsVisibleTarget() && FVector::Dist(C->GetActorLocation(), P->GetActorLocation()) < 1500.f && JoeHears(C, 1500.f))
		{
			LastContact.Add(C->GetId(), H->GetZoneAt(C->GetActorLocation()));
		}
	}
}

void ATTNarrative::OnRunEnded(int32 Ending, bool bJoeEscaped)
{
	Epilogue.Reset();
	ATTGameMode* G = GM();
	ATTPlayerCharacter* P = Joe();
	if (!G)
	{
		return;
	}
	const tr::Ending E = static_cast<tr::Ending>(Ending);
	// Route closing beats only after RunRules confirmed the exit (can_close_route).
	struct FClose { tr::Ending E; const TCHAR* Scene; const TCHAR* Exit; const TCHAR* Fact; const TCHAR* Text; };
	const FClose Closes[] = {
		{ tr::Ending::GarageEscape, TEXT("N28"), TEXT("X_GARAGE"), TEXT("garage_escaped"), TEXT("A rua cheira a chuva e a borracha queimada. As sirenes continuam lá longe. O cordão policial também.") },
		{ tr::Ending::RoofEscape, TEXT("N29"), TEXT("X_ROOF"), TEXT("roof_escaped"), TEXT("Lá de cima, o hospital parece pequeno. O sinal no céu continua aceso.") },
		{ tr::Ending::TunnelEscape, TEXT("N30"), TEXT("X_TUNNEL"), TEXT("tunnel_escaped"), TEXT("A galeria cospe você num canal, do outro lado do cordão. Ninguém viu. Por enquanto.") },
		{ tr::Ending::WindowEscape, TEXT("N31"), TEXT("X_WINDOW"), TEXT("window_escaped"), TEXT("A passarela range. Atrás de você, a janela fica aberta para um prédio vazio. Não dá para voltar.") },
	};
	for (const FClose& Close : Closes)
	{
		if (E != Close.E)
		{
			continue;
		}
		SetFact(Close.Fact);
		State->NodeOverride = Close.Exit;
		FTTNarrativeFacts Facts;
		FillFacts(Facts);
		bool bClose = false;
		try { bClose = tn::CanCloseRoute(Std(Close.Scene), Facts.F); } catch (...) {}
		if (bClose)
		{
			MarkSeen(Close.Scene);
			Epilogue.Add({ FString(), Close.Text, false });
		}
	}
	if (E == tr::Ending::SurvivedDawn) SetFact(TEXT("dawn_victory"));
	if (E == tr::Ending::HunterAbandoned) SetFact(TEXT("hide_victory"));
	if ((E == tr::Ending::SurvivedDawn || E == tr::Ending::HunterAbandoned) && Eligible(TEXT("N32")))
	{
		MarkSeen(TEXT("N32"));
	}
	if ((E == tr::Ending::Confrontation || E == tr::Ending::Destruction) && Eligible(TEXT("N33")))
	{
		MarkSeen(TEXT("N33"));
		Epilogue.Add({ FString(), E == tr::Ending::Confrontation
			? TEXT("Vocês caíram em cima dele ao mesmo tempo. Por alguns segundos, o homem no teto ficou no chão. Foi o bastante para correr. Ninguém saiu inteiro.")
			: TEXT("O hospital desabou sobre ele. Sobre tudo. Ao amanhecer, a polícia encontra Joe nos túneis, coberto de pó e sangue. Algemas. Vitória amarga."), false });
	}

	// Companions: verified evidence only (ending_rules).
	std::map<std::string, tn::CompanionEvidence> Evidence;
	TMap<ETTCompanionId, FString> Last;
	for (ETTCompanionId Id : { ETTCompanionId::Harley, ETTCompanionId::Bartholomeu, ETTCompanionId::Jack })
	{
		tn::CompanionEvidence Ev;
		const ATTCompanion* C = Companion(Id);
		const bool bNear = C && P && C->IsActive() && FVector::Dist(C->GetActorLocation(), P->GetActorLocation()) < (E == tr::Ending::Confrontation ? 2000.f : 1500.f);
		const bool bWithJoe = bNear && C->GetState() == ETTCompanionState::Following;
		Ev.togetherAtJoeExit = bJoeEscaped && bWithJoe;
		Ev.separateEscapeVerified = !Ev.togetherAtJoeExit && EscapeVerified.Contains(Id);
		Ev.captureVerified = !Ev.togetherAtJoeExit && !Ev.separateEscapeVerified && CaptureVerified.Contains(Id);
		if (const FString* L = LastContact.Find(Id))
		{
			Ev.lastContactNode = Std(*L);
			Last.Add(Id, *L);
		}
		Evidence[Std(CompanionKey(Id))] = Ev;
	}
	std::map<std::string, std::string> Ids;
	try
	{
		const tn::Difficulty D = static_cast<tn::Difficulty>(static_cast<int32>(Difficulty));
		Ids = tn::ResolveEpilogue(D, tn::DeriveOutcomes(Evidence, bJoeEscaped), HasSeen(TEXT("N34")), HasSeen(TEXT("J4")));
	}
	catch (...)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TT] Epilogue evidence rejected; showing unknown fates"));
	}
	auto Id = [&Ids](const char* Key) { auto It = Ids.find(Key); return It == Ids.end() ? FString() : FString(UTF8_TO_TCHAR(It->second.c_str())); };
	auto LastPlace = [&Last](ETTCompanionId Who)
	{
		const FString* L = Last.Find(Who);
		return L ? PlaceOfNode(*L) : FString(TEXT("logo depois que Thomas sumiu"));
	};

	const FString Harley = Id("harley");
	FString HarleyText;
	if (Harley == TEXT("E_HARLEY_TOGETHER_ASYMMETRIC")) HarleyText = TEXT("Saiu com você. Por um tempo, não fala nada. Depois: \"Você voltou por mim. Mas eu perdi a conta das vezes que você não me ouviu.\"");
	else if (Harley == TEXT("E_HARLEY_TOGETHER_RECIPROCAL")) HarleyText = TEXT("Saiu com você. Você pergunta se ela está bem antes de perguntar qualquer outra coisa. Ela demora a responder. \"Você perguntou primeiro.\"");
	else if (Harley == TEXT("E_HARLEY_ALONE")) HarleyText = TEXT("Saiu sozinha. Você sabe que ela está viva. Não sabe se ela vai atender quando você ligar.");
	else if (Harley == TEXT("E_HARLEY_CAPTURED")) HarleyText = TEXT("Foi levada. A última coisa que você ouviu dela foi um grito no escuro.");
	else if (Harley == TEXT("E_HARLEY_DEAD")) HarleyText = TEXT("Não saiu do hospital.");
	else HarleyText = FString::Printf(TEXT("Ninguém sabe onde ela está. Você a viu pela última vez %s. Desconhecido não é morto. É só desconhecido."), *LastPlace(ETTCompanionId::Harley));
	Epilogue.Add({ TEXT("HARLEY"), HarleyText, false });

	const FString Barth = Id("bartholomeu");
	FString BarthText;
	if (Barth == TEXT("E_BARTH_TOGETHER")) BarthText = AmmoGiven.Contains(ETTCompanionId::Bartholomeu) ? TEXT("Saiu com você. Devolve o carregador que você deu, vazio. \"Usei. Valeu, Joe.\"") : TEXT("Saiu com você, ainda tremendo. O medo não passa só porque a porta abriu.");
	else if (Barth == TEXT("E_BARTH_ALONE")) BarthText = TEXT("Fugiu por conta própria. Você nunca vai saber se ele teria entregado você.");
	else if (Barth == TEXT("E_BARTH_CAPTURED")) BarthText = TEXT("Foi pego. Você ouviu acontecer e não voltou.");
	else if (Barth == TEXT("E_BARTH_DEAD")) BarthText = TEXT("Não saiu do hospital.");
	else BarthText = FString::Printf(TEXT("Sumiu na noite. Último contato: %s."), *LastPlace(ETTCompanionId::Bartholomeu));
	Epilogue.Add({ TEXT("BARTHOLOMEU"), BarthText, false });

	const FString Jack = Id("jack");
	FString JackText;
	if (Jack == TEXT("E_JACK_TOGETHER")) JackText = TEXT("Saiu com você. Riu baixo quando a rua apareceu. Ninguém perguntou do quê.");
	else if (Jack == TEXT("E_JACK_ALONE")) JackText = TEXT("Saiu sozinho, pela janela. Deixou o caminho aberto para você. Ou deixou você para trás.");
	else if (Jack == TEXT("E_JACK_CAPTURED")) JackText = TEXT("Foi levado. Não gritou.");
	else if (Jack == TEXT("E_JACK_DEAD")) JackText = TEXT("Não saiu do hospital.");
	else JackText = FString::Printf(TEXT("Desapareceu. Você o viu pela última vez %s. De algum jeito, isso não te surpreende."), *LastPlace(ETTCompanionId::Jack));
	const FString Secret = Id("jack_secret");
	if (Secret == TEXT("E_JACK_SECRET_REVEALED")) JackText += TEXT(" Você sabe quem ele era. O homem no teto nunca foi o único monstro naquele hospital.");
	else if (Secret == TEXT("E_JACK_SECRET_NOT_WITNESSED")) JackText += TEXT(" As risadas dele continuam na sua cabeça. Fora de hora. Longas demais.");
	Epilogue.Add({ TEXT("JACK"), JackText, false });

	const FString ThomasId = Id("thomas");
	if (ThomasId == TEXT("E_THOMAS_JOE_KNOWS"))
	{
		Epilogue.Add({ TEXT("THOMAS"), TEXT("Você sabe o que aconteceu com ele. Você viu o relógio. Você viu a fita. Ele não foi preso. Foi morto."), false });
	}
	else if (ThomasId == TEXT("E_THOMAS_CODA_PLAYER_ONLY"))
	{
		Epilogue.Add({ TEXT("O QUE JOE NUNCA SOUBE"), TEXT("Fita da câmera do 4º andar, 02:41: uma figura escura arrasta Thomas pelo corredor oeste. 02:43: Thomas para de se mexer. No necrotério do subsolo, sob um lençol, um relógio riscado. Joe foi embora sem saber."), true });
	}
	else
	{
		Epilogue.Add({ TEXT("THOMAS"), TEXT("Nunca foi encontrado. Você não sabe o que aconteceu com ele."), false });
	}
	UE_LOG(LogTemp, Log, TEXT("[TT] Epilogue: harley=%s barth=%s jack=%s thomas=%s secret=%s"), *Harley, *Barth, *Jack, *ThomasId, *Secret);
}

void ATTNarrative::DebugTeleportCompanionToJoe(ATTCompanion* C, float Distance)
{
	ATTPlayerCharacter* P = Joe();
	if (C && P)
	{
		const FVector Loc = P->GetActorLocation() + P->GetActorForwardVector() * Distance;
		C->DebugRelease();
		C->SetActorLocation(FVector(Loc.X, Loc.Y, P->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		C->ScriptFace(P->GetActorLocation());
	}
}

// ---------------------------------------------------------------------------

void ATTNarrative::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ATTGameMode* G = GM();
	if (!G || !G->IsPlaying())
	{
		return;
	}
	ChapterTime += DeltaSeconds;
	UpdateLines(DeltaSeconds);
	if (Stage != EStage::Free)
	{
		UpdateIntro(DeltaSeconds);
	}
	if (HunterWake > 0.f)
	{
		HunterWake -= DeltaSeconds;
		if (HunterWake <= 0.f)
		{
			if (ATTHunter* Hunter = G->GetHunter())
			{
				Hunter->Activate();
			}
		}
	}
	PollTimer -= DeltaSeconds;
	if (PollTimer <= 0.f)
	{
		PollTimer = 0.25f;
		UpdateWorld();
		UpdateActs();
		UpdateEvidence();
		UpdateConfrontationHint();
	}
	UpdateRobin(DeltaSeconds);
}
