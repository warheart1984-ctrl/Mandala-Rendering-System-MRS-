#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "IntentRecord.h"
#include "Decision.h"

struct GOVERNEDENGINE_API FPolicy
{
	FString Id;
	FString Scope;
	FString Condition;
	FString Rule;
	FString Severity;
	FString Message;
	FString Param;
	FString Modifier;
	TArray<FString> Require;
};

struct GOVERNEDENGINE_API FPolicySet
{
	FString WorldId;
	TArray<FPolicy> Policies;
};

struct GOVERNEDENGINE_API FDecisionPrecedent
{
	FString Id;
	FString IntentType;
	FString WorldId;
	bool bAllowed = false;
	float DriftScore = 0.f;
};

class GOVERNEDENGINE_API FCKL
{
public:
	void LoadPoliciesFromJson(const FString& JsonText);
	FPolicySet GetPoliciesForWorld(const FWorldId& World) const;
	TArray<FDecisionPrecedent> GetPrecedents(const FIntentRecord& Intent) const;
	void RecordPrecedent(const FIntentRecord& Intent, const FDecision& Decision);

private:
	TArray<FPolicy> Policies;
	TArray<FDecisionPrecedent> Precedents;
};
