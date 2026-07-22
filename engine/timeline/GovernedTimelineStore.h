#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "GovernedTimelineDto.h"

class GOVERNEDENGINE_API FGovernedTimelineStore
{
public:
	static FGovernedTimelineDto Load(const FString& TimelineId, const FString& BasePath);
	static void ClearCache();

private:
	static TMap<FString, FGovernedTimelineDto> Cache;
};
