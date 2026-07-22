#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

struct GOVERNEDENGINE_API FFrameProvenance
{
	FString IntentId;
	FString TimelineId;
	FString WorldId;
	double TimeSeconds = 0.0;
	TMap<FString, double> Parameters;
};

class GOVERNEDENGINE_API FProvenanceRecorder
{
public:
	static void Record(const FFrameProvenance& Frame);
	static const TArray<FFrameProvenance>& GetFrames();
	static void Clear();

private:
	static TArray<FFrameProvenance> Frames;
};
