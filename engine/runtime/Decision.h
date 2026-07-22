#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

struct GOVERNEDENGINE_API FEvidenceBundle
{
	bool bEmpty = true;
	FString Timestamp;
	TMap<FString, FString> Fields;

	static FEvidenceBundle Empty()
	{
		FEvidenceBundle E;
		E.bEmpty = true;
		return E;
	}
};

struct GOVERNEDENGINE_API FDecision
{
	bool bAllowed = false;
	FString Verdict;
	FString Reason;
	FString DecisionId;
	TArray<FString> Violations;
	bool bAttachProvenance = false;
	TMap<FString, float> ParamAdjust;
};

struct GOVERNEDENGINE_API FWorldId
{
	FString Value;
	FWorldId() = default;
	explicit FWorldId(const FString& In) : Value(In) {}
};
