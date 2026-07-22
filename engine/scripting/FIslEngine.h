#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "IntentRecord.h"

class GOVERNEDENGINE_API FIslEngine
{
public:
	static FIntentRecord CompileAndEvaluate(const FString& Source, const FString& ContextJson);
};
