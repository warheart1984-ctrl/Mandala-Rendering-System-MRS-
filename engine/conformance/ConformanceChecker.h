#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

/**
 * Engine-agnostic conformance evaluator (C++ mirror).
 * Each host implements FRuntimeAdapter probes.
 * Status: interface — portable conformance evaluation.
 */

struct GOVERNEDENGINE_API FCheckResult
{
	FString Id;
	FString Domain;
	bool bPass = false;
	FString Reason;
};

struct GOVERNEDENGINE_API FConformanceReport
{
	FString Runtime;
	FString ProfileVersion;
	FString Timestamp;
	bool bCompliant = false;
	int32 Total = 0;
	int32 Passed = 0;
	int32 Failed = 0;
	TArray<FCheckResult> Results;
};

struct GOVERNEDENGINE_API FConformanceCheckDef
{
	FString Id;
	FString Domain;
	FString Description;
	FString Severity;
};

/** Probe returns pass + optional reason. */
using FConformanceProbe = TFunction<TPair<bool, FString>()>;

/** Each host fills a probe map keyed by check id. */
class GOVERNEDENGINE_API FRuntimeAdapter
{
public:
	TMap<FString, FConformanceProbe> Probes;
};

class GOVERNEDENGINE_API FConformanceChecker
{
public:
	static FConformanceReport Evaluate(
		const FString& RuntimeName,
		const TArray<FConformanceCheckDef>& Checks,
		const FRuntimeAdapter& Adapter);
};
