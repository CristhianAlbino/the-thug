#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TTAudio.h"
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
	void PlayerEscaped();

	// World events
	void ReportNoise(const FVector& Location, float Loudness, AActor* NoiseInstigator);
	void OnPlayerHid(ATTLocker* Locker);

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
	TObjectPtr<USoundWaveProcedural> AmbientWave;

	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientComponent;

	FTTDroneGenerator Drone;

	ETTGamePhase Phase = ETTGamePhase::Title;
	float PhaseTime = 0.f;
	float RunTime = 0.f;
	bool bSkipTitle = false;
	bool bCapturedInLocker = false;
	bool bDebug = false;

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
	float AutoTime = 0.f;
	int32 AutoStep = 0;
};
