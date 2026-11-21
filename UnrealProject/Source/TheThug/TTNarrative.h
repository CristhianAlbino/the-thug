#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "TTAudio.h"
#include "TTCompanion.h"
#include "TTPath.h"
#include "TTTypes.h"
#include "TTNarrative.generated.h"

class ATTGameMode;
class ATTHospital;
class ATTPlayerCharacter;
class ATTStation;
class USpotLightComponent;
class UStaticMeshComponent;
struct FTTNarrativeState;

UENUM()
enum class ETTFigure : uint8
{
	Thomas,
	Robin
};

/** Thomas in the opening and Robin's episode in the tunnels: scripted, never a second Hunter. */
UCLASS()
class THETHUG_API ATTStoryFigure : public ACharacter
{
	GENERATED_BODY()

public:
	ATTStoryFigure();
	void Setup(ETTFigure InKind);
	virtual void Tick(float DeltaSeconds) override;

	void WalkTo(const FVector& Goal, float Speed);
	bool HasArrived() const { return !Mover.HasPath(); }
	/** N03: pulled up into the dark. */
	void StartLift();
	bool IsLifted() const { return LiftTime > 1.2f; }
	ETTFigure GetKind() const { return Kind; }
	bool CanSee(const AActor* Target, float Range) const;

private:
	ETTFigure Kind = ETTFigure::Thomas;
	FTTPathFollower Mover;
	float Speed = 220.f;
	float LiftTime = -1.f;
	float StepTimer = 0.f;

	UPROPERTY()
	TObjectPtr<USpotLightComponent> Torch;
};

/** One line of dialogue. Joe only hears it when the speaker is near (or it is Joe / a radio he holds). */
struct FTTLine
{
	FString Speaker;
	FString Text;
	float Duration = 3.f;
	TWeakObjectPtr<AActor> Source;
	ETTSfx Sfx = ETTSfx::Click;
	bool bSfx = false;
	/** Opening scene: the group is together, every line reaches Joe. */
	bool bAlways = false;
};

/** A block of the end screen (GDD: epilogues per person, Thomas coda for the player only). */
struct FTTEpilogueBlock
{
	FString Title;
	FString Text;
	bool bCoda = false;
};

/**
 * Narrative director (M07). Owns no rules: every gate comes from Tools/NarrativeCore
 * (C++ port of the JSON contract), every escape result from RunRules. It watches the
 * world (zones, run flags, companions, the Hunter) and plays authored beats when the
 * contract says they are eligible.
 */
UCLASS()
class THETHUG_API ATTNarrative : public AActor
{
	GENERATED_BODY()

public:
	ATTNarrative();
	virtual void Tick(float DeltaSeconds) override;

	enum class EIntro : uint8 { Play, SkipInPlace, SkipScattered };
	void Begin(EIntro Mode);
	bool IsIntroRunning() const { return Stage != EStage::Free; }
	void SkipIntro();

	// Dialogue
	void Queue(const FString& Speaker, const FString& Text, float Duration, AActor* Source = nullptr);
	void QueueSfx(ETTSfx Sfx, AActor* Source);
	bool GetCurrentLine(FString& OutSpeaker, FString& OutText, float& OutAlpha) const;
	bool IsTalking() const { return Lines.Num() > 0; }

	// Documents
	bool IsReading() const { return !ReadingId.IsEmpty(); }
	void CloseDocument();
	FString GetReadingTitle() const;
	FString GetReadingBody() const;

	// Chapter card
	int32 GetAct() const { return Act; }
	bool GetChapterCard(FString& OutTitle, float& OutAlpha) const;

	// Hooks from the game
	FString GetStationPrompt(const ATTStation* Station) const;
	void UseStation(ATTStation* Station, ATTPlayerCharacter* Joe);
	/** Returns true when an authored scene played instead of the generic talk. */
	bool TryCompanionScene(ATTCompanion* Companion, ATTPlayerCharacter* Joe);
	void OnCompanionTaken(ATTCompanion* Companion, bool bJoeHeard);
	void OnCompanionEscaped(ATTCompanion* Companion, bool bJoeHeard);
	void OnHunterStunned();
	void OnAmmoGiven(ETTCompanionId Id) { AmmoGiven.Add(Id); }
	void OnRadioContact();
	/** Called when RunRules reports an ending; builds the epilogue. */
	void OnRunEnded(int32 Ending, bool bJoeEscaped);
	const TArray<FTTEpilogueBlock>& GetEpilogue() const { return Epilogue; }
	bool HasSeen(const FString& Id) const;
	bool HasFact(const FString& Fact) const;

	/** Tests. */
	ATTStoryFigure* GetRobin() const { return Robin; }
	void DebugTeleportCompanionToJoe(ATTCompanion* Companion, float Distance = 180.f);
	void DebugSetFact(const FString& Fact);

private:
	enum class EStage : uint8 { Siege, ToWindow, Signal, Return, Rupture, Free };

	ATTGameMode* GM() const;
	ATTHospital* Hospital() const;
	ATTPlayerCharacter* Joe() const;
	ATTCompanion* Companion(ETTCompanionId Id) const;
	FString CompanionKey(ETTCompanionId Id) const;

	void SpawnStoryDevices();
	void SpawnBatSignal();
	void ShowBatSignal(bool bFlash);
	void UpdateIntro(float DeltaSeconds);
	void Scatter(bool bTeleport);
	void UpdateLines(float DeltaSeconds);
	void UpdateWorld();
	void UpdateActs();
	void UpdateRobin(float DeltaSeconds);
	void UpdateEvidence();
	void UpdateConfrontationHint();

	/** Builds the NarrativeCore facts from the current world. */
	bool PlayCompanionScene(ATTCompanion* C, ATTPlayerCharacter* P);
	void FillFacts(struct FTTNarrativeFacts& Out) const;
	bool Eligible(const FString& Scene) const;
	void MarkSeen(const FString& Scene);
	/** Plays the authored lines of a world beat (N04, N05...). */
	void PlayBeat(const FString& Scene);
	void SetFact(const FString& Fact);
	void OpenDocument(const FString& Id);
	bool JoeHears(const AActor* Source, float Range = 900.f) const;

	TSharedPtr<FTTNarrativeState> State;
	ETTDifficulty Difficulty = ETTDifficulty::Normal;
	EStage Stage = EStage::Free;
	float StageTime = 0.f;
	float HunterWake = -1.f;
	int32 Act = 1;
	FString ChapterTitle;
	float ChapterTime = 100.f;
	FString JoeNode;
	TSet<FString> Visited;
	float PollTimer = 0.f;

	TArray<FTTLine> Lines;
	float LineTime = 0.f;
	FString CurrentSpeaker;
	FString CurrentText;
	float CurrentDuration = 0.f;
	float CurrentTime = 100.f;

	FString ReadingId;
	TArray<FTTEpilogueBlock> Epilogue;

	// Evidence for the epilogues (verified by Joe, never guessed).
	TMap<ETTCompanionId, FString> LastContact;
	TSet<ETTCompanionId> CaptureVerified;
	TSet<ETTCompanionId> EscapeVerified;
	TSet<ETTCompanionId> AmmoGiven;
	int32 ChargesPlanted = 0;
	bool bConfrontHintSaid = false;
	float CompanionRiskAt = -1000.f;

	// Robin (Punitive, once, in the tunnels)
	float TunnelTime = 0.f;
	int32 RobinStage = 0;
	float RobinTime = 0.f;

	UPROPERTY()
	TObjectPtr<ATTStoryFigure> Thomas;

	UPROPERTY()
	TObjectPtr<ATTStoryFigure> Robin;

	UPROPERTY()
	TObjectPtr<USceneComponent> SignalRoot;
};
