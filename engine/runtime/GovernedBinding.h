#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

class AActor;
class UActorComponent;
class UWorld;

struct GOVERNEDENGINE_API FGovernedBinding
{
	FString BindingId;
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<UActorComponent> Component;
};

/** Resolve timeline track.binding ids → live actors/components in World. */
class GOVERNEDENGINE_API FBindingResolver
{
public:
	static TMap<FString, FGovernedBinding> Resolve(const class FGovernedTimelineDto& Timeline, UWorld* World);
};
