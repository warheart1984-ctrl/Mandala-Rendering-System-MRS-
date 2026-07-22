#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

struct GOVERNEDENGINE_API FGovernedComponentDto
{
	FString Type;
	TMap<FString, FString> Config;
};

struct GOVERNEDENGINE_API FGovernedEntityDto
{
	FString Id;
	FString Name;
	FString Parent;
	TArray<FGovernedComponentDto> Components;
};

struct GOVERNEDENGINE_API FGovernedAssetDto
{
	FString Id;
	FString Type;
	FString Uri;
	TArray<FString> Evidence;
};

struct GOVERNEDENGINE_API FGovernedWorldDto
{
	FString Id;
	FString Name;
	FString Constitution;
	TArray<FGovernedAssetDto> Assets;
	TArray<FGovernedEntityDto> Entities;
	TArray<FString> Timelines;

	static FGovernedWorldDto FromJson(const FString& Json);
};
