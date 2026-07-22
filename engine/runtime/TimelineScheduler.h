#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "GovernedTimelineDto.h"
#include "GovernedBinding.h"

/**
 * Option B: governed timeline JSON is source of truth; Sequencer optional.
 * Tick applies active clips to bound FourD / camera actors.
 */
class GOVERNEDENGINE_API FTimelineScheduler
{
public:
	static void Run(const FGovernedTimelineDto& Timeline,
		const TMap<FString, FGovernedBinding>& Bindings,
		UWorld* World);
	static void Tick(float DeltaTime);
	static void Stop();
	static bool IsPlaying();

private:
	static FGovernedTimelineDto ActiveTimeline;
	static TMap<FString, FGovernedBinding> ActiveBindings;
	static TWeakObjectPtr<UWorld> ActiveWorld;
	static float TimeSec;
	static bool bPlaying;
};
