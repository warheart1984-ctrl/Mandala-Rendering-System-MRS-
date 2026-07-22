#pragma once

#include "CoreMinimal.h"
#include "IntentRecord.h"
#include "Decision.h"

/**
 * Movie Render Queue path — Apple ProRes .mov container (first-party MRQ encode).
 * Requires Editor + MovieRenderPipeline plugin. Falls back to viewport PNG if MRQ unavailable.
 * Status: partial — Editor-only; PIE/MRQ not CI-verified.
 */
class GOVERNEDENGINEEDITOR_API FGovernedMovieRenderQueue
{
public:
	/** Start MRQ job capturing the current PIE/editor world for Seconds @ Fps into Saved/Movies. */
	static bool StartProResCapture(
		const FIntentRecord& Intent,
		const FDecision& Decision,
		float Seconds,
		int32 Fps,
		FString& OutContainerPath,
		FString& OutSessionDir);
};
