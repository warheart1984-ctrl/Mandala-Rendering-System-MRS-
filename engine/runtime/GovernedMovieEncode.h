#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"
#include "IntentRecord.h"
#include "Decision.h"

/**
 * Editor modules (MRQ) bind this to provide true container encodes.
 * Runtime falls back to viewport PNG when unbound.
 */
DECLARE_DELEGATE_RetVal_FourParams(
	bool,
	FGovernedMovieContainerEncode,
	const FIntentRecord& /*Intent*/,
	const FDecision& /*Decision*/,
	float /*Seconds*/,
	int32 /*Fps*/);

GOVERNEDENGINE_API FGovernedMovieContainerEncode& GetGovernedMovieContainerEncode();
