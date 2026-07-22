#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "GovernedWorldDto.h"

class UWorld;

class GOVERNEDENGINE_API FGovernedWorldLoader
{
public:
	static FGovernedWorldDto LoadFromFile(const FString& Path);
	static void InstantiateWorld(const FGovernedWorldDto& WorldDto, UWorld* World);
};
