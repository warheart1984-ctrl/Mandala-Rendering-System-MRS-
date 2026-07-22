#pragma once

#include "CoreMinimal.h"
#include "ConformanceChecker.h"
#include "CKL.h"

/** Builds FRuntimeAdapter probes mirroring BrowserRuntimeAdapter.js */
class GOVERNEDENGINE_API FUnrealRuntimeAdapter
{
public:
	static FRuntimeAdapter Build(const FPolicySet& PolicySet);
	static FPolicySet LoadDefaultPolicies();
};
