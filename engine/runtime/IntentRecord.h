// Canonical Unreal API surface — implementations live in
// unreal/GovernedEnginePlugin/Source/GovernedEngine/
#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

struct GOVERNEDENGINE_API FIntentRecord
{
	FString Id;
	FString Actor;
	FString Type;
	FString Kind;
	FString World;
	FString TimelineId;
	FString Entity;
	FString EvidenceId;
	FString At;
	FString Timestamp;
	FString Source;
	FString Goal;
};
