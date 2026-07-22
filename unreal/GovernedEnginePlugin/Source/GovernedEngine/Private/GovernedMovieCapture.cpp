#include "GovernedMovieCapture.h"
#include "CssvRegistry.h"
#include "TimelineExecutor.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UGovernedMovieCaptureComponent::UGovernedMovieCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGovernedMovieCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGovernedMovieCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureTimer);
	}
	Super::EndPlay(EndPlayReason);
}

UGovernedMovieCaptureComponent* UGovernedMovieCaptureComponent::FindInWorld(UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (UGovernedMovieCaptureComponent* Comp = (*It)->FindComponentByClass<UGovernedMovieCaptureComponent>())
		{
			return Comp;
		}
	}
	return nullptr;
}

bool FGovernedMovieCapture::TryStart(const FIntentRecord& Intent, const FDecision& Decision, const FEvidenceBundle& Evidence)
{
	UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
	if (!World && GEngine)
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game)
			{
				World = Ctx.World();
				break;
			}
		}
	}
	UGovernedMovieCaptureComponent* Comp = UGovernedMovieCaptureComponent::FindInWorld(World);
	if (!Comp)
	{
		UE_LOG(LogTemp, Error, TEXT("[MovieCapture] No UGovernedMovieCaptureComponent in play world"));
		return false;
	}
	return Comp->StartGovernedRecord(Intent, Decision, Evidence);
}

bool UGovernedMovieCaptureComponent::StartGovernedRecord(
	const FIntentRecord& Intent,
	const FDecision& Decision,
	const FEvidenceBundle& Evidence)
{
	if (bIsRecording)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MovieCapture] Already recording"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	float Seconds = DefaultSeconds;
	int32 Fps = DefaultFps;
	if (const FString* Sec = Evidence.Fields.Find(TEXT("seconds")))
	{
		Seconds = FCString::Atof(**Sec);
	}
	if (const FString* FpsStr = Evidence.Fields.Find(TEXT("fps")))
	{
		Fps = FCString::Atoi(**FpsStr);
	}
	Fps = FMath::Clamp(Fps, 1, 120);
	Seconds = FMath::Max(0.1f, Seconds);

	if (!Intent.TimelineId.IsEmpty())
	{
		FTimelineExecutor::Play(Intent);
	}

	ActiveIntent = Intent;
	ActiveDecision = Decision;
	ActiveFps = Fps;
	FrameInterval = 1.f / static_cast<float>(Fps);
	SecondsRemaining = Seconds;
	FrameIndex = 0;
	FrameFiles.Reset();

	const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
	SessionName = FString::Printf(TEXT("%s-%s"), *Basename, *Stamp);
	OutputDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Movies"), SessionName);
	IFileManager::Get().MakeDirectory(*OutputDir, true);

	bIsRecording = true;
	UE_LOG(LogTemp, Log, TEXT("[MovieCapture] Recording %.1fs @ %dfps → %s"), Seconds, Fps, *OutputDir);

	World->GetTimerManager().SetTimer(
		CaptureTimer,
		FTimerDelegate::CreateUObject(this, &UGovernedMovieCaptureComponent::CaptureTick),
		FrameInterval,
		true);

	return true;
}

void UGovernedMovieCaptureComponent::CaptureTick()
{
	if (!bIsRecording)
	{
		return;
	}

	const FString FileName = FString::Printf(TEXT("frame_%05d.png"), FrameIndex);
	const FString FullPath = FPaths::Combine(OutputDir, FileName);
	int32 Width = 0;
	int32 Height = 0;
	if (CaptureViewportPng(FullPath, Width, Height))
	{
		FrameFiles.Add(FileName);
		FrameIndex++;
	}

	SecondsRemaining -= FrameInterval;
	if (SecondsRemaining <= 0.f)
	{
		FinishRecording();
	}
}

bool UGovernedMovieCaptureComponent::CaptureViewportPng(const FString& FilePath, int32& OutWidth, int32& OutHeight)
{
	if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		return false;
	}

	FViewport* Viewport = GEngine->GameViewport->Viewport;
	TArray<FColor> Bitmap;
	if (!Viewport->ReadPixels(Bitmap))
	{
		return false;
	}

	OutWidth = Viewport->GetSizeXY().X;
	OutHeight = Viewport->GetSizeXY().Y;
	if (OutWidth <= 0 || OutHeight <= 0 || Bitmap.Num() < OutWidth * OutHeight)
	{
		return false;
	}

	// Viewport bitmaps are top-down; PNG expects bottom-up flip for correct orientation.
	TArray<FColor> Flipped;
	Flipped.SetNumUninitialized(Bitmap.Num());
	for (int32 Y = 0; Y < OutHeight; ++Y)
	{
		const int32 SrcRow = Y * OutWidth;
		const int32 DstRow = (OutHeight - 1 - Y) * OutWidth;
		FMemory::Memcpy(&Flipped[DstRow], &Bitmap[SrcRow], OutWidth * sizeof(FColor));
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Png = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!Png.IsValid())
	{
		return false;
	}

	if (!Png->SetRaw(Flipped.GetData(), Flipped.Num() * sizeof(FColor), OutWidth, OutHeight, ERGBFormat::BGRA, 8))
	{
		return false;
	}

	const TArray64<uint8>& Compressed = Png->GetCompressed(100);
	TArray<uint8> Bytes;
	Bytes.Append(Compressed.GetData(), static_cast<int32>(Compressed.Num()));
	return FFileHelper::SaveArrayToFile(Bytes, *FilePath);
}

void UGovernedMovieCaptureComponent::FinishRecording()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureTimer);
	}

	int32 Width = 0;
	int32 Height = 0;
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		Width = GEngine->GameViewport->Viewport->GetSizeXY().X;
		Height = GEngine->GameViewport->Viewport->GetSizeXY().Y;
	}

	WriteManifestAndProvenance(Width, Height);

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("format"), TEXT("png-sequence"));
	Payload->SetStringField(TEXT("outputDir"), OutputDir);
	Payload->SetNumberField(TEXT("frameCount"), FrameFiles.Num());
	Payload->SetNumberField(TEXT("fps"), ActiveFps);
	FCssvRegistry::Get().RegisterArtifact(
		FString::Printf(TEXT("movie-%s"), *SessionName),
		TEXT("movie"),
		Payload);

	bIsRecording = false;
	UE_LOG(LogTemp, Log, TEXT("[MovieCapture] Done — %d frames. Dir: %s"), FrameFiles.Num(), *OutputDir);
}

void UGovernedMovieCaptureComponent::WriteManifestAndProvenance(int32 Width, int32 Height)
{
	TSharedPtr<FJsonObject> Manifest = MakeShared<FJsonObject>();
	Manifest->SetStringField(TEXT("format"), TEXT("png-sequence"));
	Manifest->SetStringField(TEXT("hostId"), TEXT("unreal"));
	Manifest->SetStringField(TEXT("intentId"), ActiveIntent.Id);
	Manifest->SetStringField(TEXT("worldId"), ActiveIntent.World);
	Manifest->SetStringField(TEXT("timelineId"), ActiveIntent.TimelineId);
	Manifest->SetStringField(TEXT("decisionId"), ActiveDecision.DecisionId);
	Manifest->SetStringField(TEXT("outputDir"), OutputDir);
	Manifest->SetStringField(TEXT("basename"), SessionName);
	Manifest->SetNumberField(TEXT("fps"), ActiveFps);
	Manifest->SetNumberField(TEXT("frameCount"), FrameFiles.Num());
	Manifest->SetNumberField(TEXT("width"), Width);
	Manifest->SetNumberField(TEXT("height"), Height);
	Manifest->SetStringField(TEXT("createdAt"), FDateTime::UtcNow().ToIso8601());

	const FString Pattern = FPaths::Combine(OutputDir, TEXT("frame_%05d.png"));
	const FString Mp4 = FPaths::Combine(OutputDir, SessionName + TEXT(".mp4"));
	Manifest->SetStringField(
		TEXT("muxHint"),
		FString::Printf(
			TEXT("ffmpeg -y -framerate %d -i \"%s\" -c:v libx264 -pix_fmt yuv420p \"%s\""),
			ActiveFps, *Pattern, *Mp4));

	TArray<TSharedPtr<FJsonValue>> FramesArr;
	for (const FString& Name : FrameFiles)
	{
		FramesArr.Add(MakeShared<FJsonValueString>(Name));
	}
	Manifest->SetArrayField(TEXT("frameFiles"), FramesArr);

	FString ManifestJson;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ManifestJson);
	FJsonSerializer::Serialize(Manifest.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(ManifestJson, *FPaths::Combine(OutputDir, TEXT("movie-manifest.json")));

	TSharedPtr<FJsonObject> Prov = MakeShared<FJsonObject>();
	Prov->SetStringField(TEXT("host"), TEXT("unreal"));
	Prov->SetStringField(TEXT("intentId"), ActiveIntent.Id);
	Prov->SetStringField(TEXT("worldId"), ActiveIntent.World);
	Prov->SetStringField(TEXT("timelineId"), ActiveIntent.TimelineId);
	Prov->SetStringField(TEXT("decisionId"), ActiveDecision.DecisionId);
	Prov->SetStringField(TEXT("verdict"), ActiveDecision.Verdict);
	Prov->SetNumberField(TEXT("frameCount"), FrameFiles.Num());
	Prov->SetNumberField(TEXT("fps"), ActiveFps);
	Prov->SetStringField(TEXT("outputDir"), OutputDir);

	FString ProvJson;
	const TSharedRef<TJsonWriter<>> ProvWriter = TJsonWriterFactory<>::Create(&ProvJson);
	FJsonSerializer::Serialize(Prov.ToSharedRef(), ProvWriter);
	FFileHelper::SaveStringToFile(ProvJson, *FPaths::Combine(OutputDir, TEXT("provenance.json")));
}
