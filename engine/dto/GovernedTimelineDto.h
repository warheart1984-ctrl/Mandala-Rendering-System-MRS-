#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

struct GOVERNEDENGINE_API FGovernedClipPayloadDto
{
	FString Param;
	float From = 0.f;
	float To = 0.f;
	float Speed = 1.f;
	TArray<FString> Planes;
};

struct GOVERNEDENGINE_API FGovernedClipDto
{
	FString Id;
	FString Start;
	FString Duration;
	float StartSec = 0.f;
	float DurationSec = 0.f;
	FString Action;
	FGovernedClipPayloadDto Payload;
};

struct GOVERNEDENGINE_API FGovernedTrackDto
{
	FString Id;
	FString Binding;
	TArray<FGovernedClipDto> Clips;
};

struct GOVERNEDENGINE_API FGovernedTimelineDto
{
	FString Id;
	FString Name;
	float DurationSec = 12.f;
	TArray<FGovernedTrackDto> Tracks;

	static FGovernedTimelineDto FromJson(const FString& Json);
};
