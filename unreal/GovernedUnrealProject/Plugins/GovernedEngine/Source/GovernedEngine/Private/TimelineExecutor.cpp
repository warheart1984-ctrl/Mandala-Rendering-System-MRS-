#include "TimelineExecutor.h"
#include "FourDRendererComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraActor.h"

namespace
{
	float JsonNumber(const TSharedPtr<FJsonObject>& Obj, const FString& Key, float DefaultValue)
	{
		if (!Obj.IsValid()) return DefaultValue;
		if (Obj->HasTypedField<EJson::Number>(Key)) return static_cast<float>(Obj->GetNumberField(Key));
		return DefaultValue;
	}

	FString ResolveTimelinePath(const FString& TimelineId)
	{
		if (TimelineId.EndsWith(TEXT(".json")))
		{
			return TimelineId;
		}
		const FString FileName = TimelineId.Contains(TEXT("opening"))
			? TEXT("opening_4d_reveal.timeline.json")
			: (TimelineId + TEXT(".timeline.json"));

		const TArray<FString> Candidates = {
			FPaths::ProjectContentDir() / TEXT("Timelines") / FileName,
			FPaths::Combine(
				FPaths::ProjectPluginsDir(),
				TEXT("GovernedEngine/Content/Timelines"),
				FileName),
		};
		for (const FString& Candidate : Candidates)
		{
			if (FPaths::FileExists(Candidate))
			{
				return Candidate;
			}
		}
		return Candidates[0];
	}
}

FGovernedTimeline FGovernedTimelineStore::Load(const FString& TimelineIdOrPath)
{
	FGovernedTimeline Timeline;
	const FString Path = ResolveTimelinePath(TimelineIdOrPath);
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *Path))
	{
		UE_LOG(LogTemp, Error, TEXT("Timeline load failed: %s"), *Path);
		Timeline.Id = TimelineIdOrPath;
		return Timeline;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Timeline JSON parse failed: %s"), *Path);
		return Timeline;
	}

	Timeline.Id = Root->GetStringField(TEXT("id"));
	Timeline.Name = Root->GetStringField(TEXT("name"));
	Timeline.DurationSec = JsonNumber(Root, TEXT("durationSec"), 12.f);

	const TArray<TSharedPtr<FJsonValue>>* TracksArr = nullptr;
	if (Root->TryGetArrayField(TEXT("tracks"), TracksArr))
	{
		for (const TSharedPtr<FJsonValue>& TV : *TracksArr)
		{
			const TSharedPtr<FJsonObject> TObj = TV->AsObject();
			if (!TObj.IsValid()) continue;
			FGovernedTrack Track;
			Track.Id = TObj->GetStringField(TEXT("id"));
			Track.Binding = TObj->GetStringField(TEXT("binding"));
			const TArray<TSharedPtr<FJsonValue>>* ClipsArr = nullptr;
			if (TObj->TryGetArrayField(TEXT("clips"), ClipsArr))
			{
				for (const TSharedPtr<FJsonValue>& CV : *ClipsArr)
				{
					const TSharedPtr<FJsonObject> CObj = CV->AsObject();
					if (!CObj.IsValid()) continue;
					FGovernedClip Clip;
					Clip.Id = CObj->GetStringField(TEXT("id"));
					Clip.StartSec = JsonNumber(CObj, TEXT("startSec"), 0.f);
					Clip.DurationSeconds = JsonNumber(CObj, TEXT("durationSec"), 0.f);
					Clip.Action = CObj->GetStringField(TEXT("action"));
					const TSharedPtr<FJsonObject> Payload = CObj->GetObjectField(TEXT("payload"));
					if (Payload.IsValid())
					{
						Clip.Payload.Param = Payload->GetStringField(TEXT("param"));
						Clip.Payload.From = JsonNumber(Payload, TEXT("from"), 0.f);
						Clip.Payload.To = JsonNumber(Payload, TEXT("to"), 1.f);
						Clip.Payload.Speed = JsonNumber(Payload, TEXT("speed"), Clip.Payload.To);
						const TArray<TSharedPtr<FJsonValue>>* Planes = nullptr;
						if (Payload->TryGetArrayField(TEXT("planes"), Planes))
						{
							for (const TSharedPtr<FJsonValue>& P : *Planes)
							{
								Clip.Payload.Planes.Add(P->AsString());
							}
						}
					}
					Track.Clips.Add(Clip);
				}
			}
			Timeline.Tracks.Add(Track);
		}
	}
	return Timeline;
}

FTimelineBindings FBindingResolver::Resolve(UWorld* World, const FGovernedTimeline& Timeline)
{
	FTimelineBindings Bindings;
	if (!World) return Bindings;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;
		const FString Name = Actor->GetName();
		if (Name.Contains(TEXT("camera-main")) || Name.Contains(TEXT("GovernedCamera")) ||
			Name.Contains(TEXT("MainCamera")) || Name.Contains(TEXT("Camera")))
		{
			Bindings.CameraActor = Actor;
		}
		UFourDRendererComponent* FourD = Actor->FindComponentByClass<UFourDRendererComponent>();
		if (FourD && (Name.Contains(TEXT("tesseract")) || Name.Contains(TEXT("TesseractHero"))))
		{
			Bindings.Tesseract = FourD;
		}
	}

	// Fallback: first FourD component in world
	if (!Bindings.Tesseract.IsValid())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UFourDRendererComponent* FourD = (*It)->FindComponentByClass<UFourDRendererComponent>())
			{
				Bindings.Tesseract = FourD;
				break;
			}
		}
	}
	return Bindings;
}

void Apply4DClip(UFourDRendererComponent* Renderer, const FGovernedClip& Clip, float LocalTimeSeconds)
{
	if (!Renderer || Clip.DurationSeconds <= 0.f) return;
	const float TNorm = FMath::Clamp(LocalTimeSeconds / Clip.DurationSeconds, 0.f, 1.f);
	if (Clip.Action == TEXT("render_4d") || Clip.Payload.Param == TEXT("speed"))
	{
		const float From = Clip.Payload.From;
		const float To = (Clip.Payload.To != 0.f) ? Clip.Payload.To : Clip.Payload.Speed;
		Renderer->Speed = FMath::Lerp(From, To, TNorm);
	}
	if (Clip.Payload.Param == TEXT("d4"))
	{
		Renderer->D4 = FMath::Lerp(Clip.Payload.From, Clip.Payload.To, TNorm);
	}
	if (Clip.Payload.Param == TEXT("d3"))
	{
		Renderer->D3 = FMath::Lerp(Clip.Payload.From, Clip.Payload.To, TNorm);
	}
}

void FTimelineScheduler::Run(UWorld* World, const FGovernedTimeline& Timeline, const FTimelineBindings& Bindings)
{
	if (!World) return;

	struct FSchedState
	{
		FGovernedTimeline Timeline;
		FTimelineBindings Bindings;
		float TimeSec = 0.f;
		FTimerHandle Handle;
	};

	TSharedRef<FSchedState> State = MakeShared<FSchedState>();
	State->Timeline = Timeline;
	State->Bindings = Bindings;

	World->GetTimerManager().SetTimer(
		State->Handle,
		FTimerDelegate::CreateLambda([World, State]()
		{
			State->TimeSec += 0.033f;
			for (const FGovernedTrack& Track : State->Timeline.Tracks)
			{
				for (const FGovernedClip& Clip : Track.Clips)
				{
					const float End = Clip.StartSec + Clip.DurationSeconds;
					if (State->TimeSec < Clip.StartSec || State->TimeSec > End) continue;
					const float Local = State->TimeSec - Clip.StartSec;
					if (Track.Binding.Contains(TEXT("tesseract")) || Clip.Action == TEXT("render_4d") ||
						Clip.Payload.Param == TEXT("speed") || Clip.Payload.Param == TEXT("d4"))
					{
						if (UFourDRendererComponent* R = State->Bindings.Tesseract.Get())
						{
							Apply4DClip(R, Clip, Local);
						}
					}
					if (Track.Binding.Contains(TEXT("camera")) || Clip.Payload.Param == TEXT("d3"))
					{
						if (UFourDRendererComponent* R = State->Bindings.Tesseract.Get())
						{
							// Camera dolly expressed as d3 in browser demo; apply same param to renderer D3
							if (Clip.Payload.Param == TEXT("d3"))
							{
								Apply4DClip(R, Clip, Local);
							}
						}
						if (AActor* Cam = State->Bindings.CameraActor.Get())
						{
							if (Clip.Payload.Param.Contains(TEXT("position")) || Clip.Id.Contains(TEXT("dolly")))
							{
								FVector Loc = Cam->GetActorLocation();
								Loc.Z = FMath::Lerp(-1000.f, -200.f, FMath::Clamp(Local / FMath::Max(Clip.DurationSeconds, 0.001f), 0.f, 1.f));
								// Keep existing XY; optional Z dolly in UE units
								Cam->SetActorLocation(Loc);
							}
						}
					}
				}
			}
			if (State->TimeSec >= State->Timeline.DurationSec)
			{
				World->GetTimerManager().ClearTimer(State->Handle);
				UE_LOG(LogTemp, Log, TEXT("Governed timeline finished: %s"), *State->Timeline.Id);
			}
		}),
		0.033f,
		true);
}

void FTimelineExecutor::Play(const FIntentRecord& Intent)
{
	UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
	if (!World)
	{
		// Fallback: try first game world
		if (GEngine)
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
	}
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("TimelineExecutor: no play world"));
		return;
	}

	const FString TimelineKey = Intent.TimelineId.IsEmpty() ? TEXT("opening_4d_reveal") : Intent.TimelineId;
	const FGovernedTimeline Timeline = FGovernedTimelineStore::Load(TimelineKey);
	const FTimelineBindings Bindings = FBindingResolver::Resolve(World, Timeline);
	FTimelineScheduler::Run(World, Timeline, Bindings);
	UE_LOG(LogTemp, Log, TEXT("TimelineExecutor playing %s for intent %s"), *Timeline.Id, *Intent.Id);
}
