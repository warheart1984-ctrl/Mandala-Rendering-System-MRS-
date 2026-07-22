#include "GovernedMovieRenderQueue.h"
#include "CssvRegistry.h"
#include "TimelineExecutor.h"
#include "GovernanceKernel.h"
#include "CKL.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelinePrimaryConfig.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelineAppleProResOutput.h"
#include "MoviePipelinePIEExecutor.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Editor.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FGovernedMovieRenderQueue::StartProResCapture(
	const FIntentRecord& Intent,
	const FDecision& Decision,
	float Seconds,
	int32 Fps,
	FString& OutContainerPath,
	FString& OutSessionDir)
{
	if (!GEditor)
	{
		UE_LOG(LogTemp, Error, TEXT("[MRQ] GEditor unavailable"));
		return false;
	}

	if (!Decision.bAllowed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MRQ] Refusing capture — decision denied"));
		return false;
	}

	UMoviePipelineQueueSubsystem* Subsystem =
		GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[MRQ] MoviePipelineQueueSubsystem missing — enable MovieRenderPipeline plugin"));
		return false;
	}

	Fps = FMath::Clamp(Fps, 1, 120);
	Seconds = FMath::Max(0.1f, Seconds);
	const int32 FrameCount = FMath::Max(1, FMath::RoundToInt(Seconds * Fps));

	const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
	const FString SessionName = FString::Printf(TEXT("4dce-movie-%s"), *Stamp);
	OutSessionDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Movies"), SessionName);
	IFileManager::Get().MakeDirectory(*OutSessionDir, true);

	ULevelSequence* Sequence = NewObject<ULevelSequence>(GetTransientPackage(), NAME_None, RF_Transient);
	Sequence->Initialize();
	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		UE_LOG(LogTemp, Error, TEXT("[MRQ] Failed to create MovieScene"));
		return false;
	}

	const FFrameRate DisplayRate(Fps, 1);
	MovieScene->SetDisplayRate(DisplayRate);
	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), FFrameNumber(FrameCount)));

	UMoviePipelineQueue* Queue = Subsystem->GetQueue();
	if (!Queue)
	{
		UE_LOG(LogTemp, Error, TEXT("[MRQ] No queue"));
		return false;
	}
	Queue->DeleteAllJobs();

	UMoviePipelineExecutorJob* Job = Queue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	Job->Sequence = FSoftObjectPath(Sequence);
	if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
	{
		Job->Map = FSoftObjectPath(EditorWorld);
	}
	Job->JobName = SessionName;

	UMoviePipelinePrimaryConfig* Config = NewObject<UMoviePipelinePrimaryConfig>(Job);
	UMoviePipelineOutputSetting* Output =
		Cast<UMoviePipelineOutputSetting>(Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
	if (Output)
	{
		Output->OutputDirectory.Path = OutSessionDir;
		Output->FileNameFormat = SessionName;
		Output->OutputResolution = FIntPoint(1920, 1080);
		Output->OutputFrameRate = DisplayRate;
		Output->bUseCustomFrameRate = true;
		Output->bUseCustomPlaybackRange = true;
		Output->CustomStartFrame = 0;
		Output->CustomEndFrame = FrameCount;
	}

	Config->FindOrAddSettingByClass(UMoviePipelineAppleProResOutput::StaticClass());
	Job->SetConfiguration(Config);

	if (!Intent.TimelineId.IsEmpty())
	{
		FTimelineExecutor::Play(Intent);
	}

	OutContainerPath = FPaths::Combine(OutSessionDir, SessionName + TEXT(".mov"));

	TSharedPtr<FJsonObject> Manifest = MakeShared<FJsonObject>();
	Manifest->SetStringField(TEXT("format"), TEXT("prores-mov"));
	Manifest->SetStringField(TEXT("hostId"), TEXT("unreal"));
	Manifest->SetStringField(TEXT("encoder"), TEXT("MovieRenderQueue"));
	Manifest->SetStringField(TEXT("intentId"), Intent.Id);
	Manifest->SetStringField(TEXT("worldId"), Intent.World);
	Manifest->SetStringField(TEXT("timelineId"), Intent.TimelineId);
	Manifest->SetStringField(TEXT("decisionId"), Decision.DecisionId);
	Manifest->SetStringField(TEXT("outputDir"), OutSessionDir);
	Manifest->SetStringField(TEXT("containerPath"), OutContainerPath);
	Manifest->SetStringField(TEXT("basename"), SessionName);
	Manifest->SetNumberField(TEXT("fps"), Fps);
	Manifest->SetNumberField(TEXT("frameCount"), FrameCount);
	Manifest->SetNumberField(TEXT("seconds"), Seconds);
	Manifest->SetStringField(TEXT("createdAt"), FDateTime::UtcNow().ToIso8601());

	FString ManifestJson;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ManifestJson);
	FJsonSerializer::Serialize(Manifest.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(ManifestJson, *FPaths::Combine(OutSessionDir, TEXT("movie-manifest.json")));

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("format"), TEXT("prores-mov"));
	Payload->SetStringField(TEXT("containerPath"), OutContainerPath);
	Payload->SetStringField(TEXT("outputDir"), OutSessionDir);
	FCssvRegistry::Get().RegisterArtifact(
		FString::Printf(TEXT("movie-%s"), *SessionName),
		TEXT("movie"),
		Payload);

	Subsystem->RenderQueueWithExecutor(UMoviePipelinePIEExecutor::StaticClass());
	UE_LOG(LogTemp, Log, TEXT("[MRQ] ProRes job started → %s (%d frames @ %dfps)"), *OutContainerPath, FrameCount, Fps);
	return true;
}
