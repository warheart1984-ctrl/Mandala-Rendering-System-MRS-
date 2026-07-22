#pragma once

#include "CoreMinimal.h"
#include "IntentRecord.h"

struct FGovernedClipPayload
{
	FString Param;
	float From = 0.f;
	float To = 0.f;
	float Speed = 1.5f;
	TArray<FString> Planes;
};

struct FGovernedClip
{
	FString Id;
	float StartSec = 0.f;
	float DurationSeconds = 0.f;
	FString Action;
	FGovernedClipPayload Payload;
};

struct FGovernedTrack
{
	FString Id;
	FString Binding;
	TArray<FGovernedClip> Clips;
};

struct FGovernedTimeline
{
	FString Id;
	FString Name;
	float DurationSec = 12.f;
	TArray<FGovernedTrack> Tracks;
};

struct FTimelineBindings
{
	TWeakObjectPtr<AActor> CameraActor;
	TWeakObjectPtr<class UFourDRendererComponent> Tesseract;
};

class GOVERNEDENGINE_API FGovernedTimelineStore
{
public:
	static FGovernedTimeline Load(const FString& TimelineIdOrPath);
};

class GOVERNEDENGINE_API FBindingResolver
{
public:
	static FTimelineBindings Resolve(UWorld* World, const FGovernedTimeline& Timeline);
};

class GOVERNEDENGINE_API FTimelineScheduler
{
public:
	static void Run(UWorld* World, const FGovernedTimeline& Timeline, const FTimelineBindings& Bindings);
};

class GOVERNEDENGINE_API FTimelineExecutor
{
public:
	static void Play(const FIntentRecord& Intent);
};
