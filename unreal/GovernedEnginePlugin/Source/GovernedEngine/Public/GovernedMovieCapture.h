#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IntentRecord.h"
#include "Decision.h"
#include "GovernedMovieCapture.generated.h"

/**
 * Engine-native movie pipeline for Unreal PIE/Game.
 * Captures viewport frames to a PNG sequence + manifest under Saved/Movies.
 * Status: partial — capture path implemented; PIE not CI-verified.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOVERNEDENGINE_API UGovernedMovieCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGovernedMovieCaptureComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	float DefaultSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	int32 DefaultFps = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	FString Basename = TEXT("4dce-movie");

	UPROPERTY(BlueprintReadOnly, Category = "Governed|Movie")
	bool bIsRecording = false;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Start after CKL allow. Plays timeline when TimelineId is set. */
	bool StartGovernedRecord(const FIntentRecord& Intent, const FDecision& Decision, const FEvidenceBundle& Evidence);

	static UGovernedMovieCaptureComponent* FindInWorld(UWorld* World);

private:
	FTimerHandle CaptureTimer;
	FString OutputDir;
	FString SessionName;
	FIntentRecord ActiveIntent;
	FDecision ActiveDecision;
	float SecondsRemaining = 0.f;
	float FrameInterval = 0.033f;
	int32 FrameIndex = 0;
	int32 ActiveFps = 30;
	TArray<FString> FrameFiles;

	void CaptureTick();
	void FinishRecording();
	bool CaptureViewportPng(const FString& FilePath, int32& OutWidth, int32& OutHeight);
	void WriteManifestAndProvenance(int32 Width, int32 Height);
};

class GOVERNEDENGINE_API FGovernedMovieCapture
{
public:
	static bool TryStart(const FIntentRecord& Intent, const FDecision& Decision, const FEvidenceBundle& Evidence);
};
