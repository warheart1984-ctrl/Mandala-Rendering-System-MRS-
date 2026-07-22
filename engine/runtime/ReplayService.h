#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "FrameProvenance.h"

class GOVERNEDENGINE_API IReplayTarget
{
public:
	virtual ~IReplayTarget() = default;
	virtual void ApplyFrame(const FFrameProvenance& Frame) = 0;
};

class GOVERNEDENGINE_API FReplayService
{
public:
	static void Replay(const TArray<FFrameProvenance>& Frames, IReplayTarget* Target)
	{
		if (!Target) return;
		for (const FFrameProvenance& Frame : Frames)
		{
			Target->ApplyFrame(Frame);
		}
	}
};
