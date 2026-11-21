#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TTAudio.h"
#include "TTTypes.h"
#include "TTGameMode.generated.h"

class ATTHospital;
class ATTHunter;
class ATTLocker;
class ATTPlayerCharacter;
class UAudioComponent;
class USoundWaveProcedural;

UENUM()
enum class ETTGamePhase : uint8
{
	Title,
	Playing,
	Paused,
	Captured,
	Escaped
};

UCLASS()
class THETHUG_API ATTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATTGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Flow
	void StartGame();
	void TogglePause();
	void RestartRun();
	void QuitGame();
	void PlayerCaptured(ATTHunter* By);
	void PlayerDied(const FString& Reason);
	void PlayerEscaped(const FString& How = FString());

	void RegisterSmoke(class ATTSmokeCloud* Smoke);
	/** True when an active smoke cloud sits between A and B (the Hunter cannot see through it). */
	bool IsSightBlockedBySmoke(const FVector& A, const FVector& B) const;
	const FString& GetEndTitle() const { return EndTitle; }
	const FString& GetEndSubtitle() const { return EndSubtitle; }

	// World events
	void ReportNoise(const FVector& Location, ETTNoise Kind, AActor* NoiseInstigator);
	void OnPlayerHid(ATTLocker* Locker);
	void OnPlayerLeftLocker(ATTLocker* Locker);

	ETTDifficulty GetDifficulty() const { return Difficulty; }
	void SetDifficulty(ETTDifficulty InDifficulty) { Difficulty = InDifficulty; }

	// UI text
	void ShowMessage(const FString& Text, float Duration);
	void ShowSubtitle(const FString& Line, float Duration);
	void SetObjective(const FString& Text);

	ETTGamePhase GetPhase() const { return Phase; }
	bool IsPlaying() const { return Phase == ETTGamePhase::Playing; }
	float GetPhaseTime() const { return PhaseTime; }
	float GetRunTime() const { return RunTime; }
	const FString& GetMessage() const { return Message; }
	float GetMessageAlpha() const;
	const FString& GetSubtitle() const { return Subtitle; }
	float GetSubtitleAlpha() const;
	const FString& GetObjective() const { return Objective; }
	float GetObjectiveAlpha() const;
	bool IsDebugEnabled() const { return bDebug; }
	bool WasCapturedInLocker() const { return bCapturedInLocker; }

	ATTHospital* GetHospital() const { return Hospital; }
	const TArray<TObjectPtr<class ATTCompanion>>& GetCompanions() const { return Companions; }
	void OnCompanionTaken(class ATTCompanion* Companion);
	ATTHunter* GetHunter() const { return Hunter; }
	ATTPlayerCharacter* GetPlayer() const;

private:
	void EnsureWorld();
	void UpdateAmbience();
	void UpdateAutomation(float DeltaSeconds);
	void SetPhase(ETTGamePhase NewPhase);

	UPROPERTY()
	TObjectPtr<ATTHospital> Hospital;

	UPROPERTY()
	TObjectPtr<ATTHunter> Hunter;

	UPROPERTY()
	TArray<TObjectPtr<class ATTCompanion>> Companions;

	bool bCompanionTest = false;
	bool bFloorsTest = false;
	void UpdateFloorsTest();
	void UpdateCompanionTest();

	UPROPERTY()
	TObjectPtr<USoundWaveProcedural> AmbientWave;

	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientComponent;

	FTTDroneGenerator Drone;

	ETTGamePhase Phase = ETTGamePhase::Title;
	ETTDifficulty Difficulty = ETTDifficulty::Normal;
	float PhaseTime = 0.f;
	float RunTime = 0.f;
	bool bSkipTitle = false;
	bool bCapturedInLocker = false;
	bool bDebug = false;

	TArray<TWeakObjectPtr<class ATTSmokeCloud>> Smokes;
	FString EndTitle;
	FString EndSubtitle;

	FString Message;
	float MessageTime = 0.f;
	float MessageDuration = 1.f;
	FString Subtitle;
	float SubtitleTime = 0.f;
	float SubtitleDuration = 1.f;
	FString Objective;
	float ObjectiveTime = 0.f;

	// "-ttshots": scripted camera positions + screenshots, then quit (used to verify builds).
	bool bAutomation = false;
	bool bAITest = false;
	bool bLearnTest = false;
	bool bSurviveTest = false;
	void UpdateSurviveTest();
	float AutoTime = 0.f;
	int32 AutoStep = 0;
};
