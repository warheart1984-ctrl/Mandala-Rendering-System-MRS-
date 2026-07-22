#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "IntentRecord.h"
#include "Decision.h"
#include "CKL.h"

class GOVERNEDENGINE_API FDecisionEngine
{
public:
	static FDecision Resolve(const FIntentRecord& Intent, const FEvidenceBundle& Evidence, const FPolicySet& Policies);
};

class GOVERNEDENGINE_API FGovernanceKernel
{
public:
	FGovernanceKernel();
	FDecision EvaluateIntent(const FIntentRecord& Intent, const FEvidenceBundle& Evidence);
	FCKL& GetCKL() { return CKL; }

private:
	FCKL CKL;
};
