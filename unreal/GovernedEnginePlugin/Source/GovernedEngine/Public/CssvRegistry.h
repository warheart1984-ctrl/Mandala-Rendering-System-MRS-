#pragma once

#include "CoreMinimal.h"
#include "FrameProvenance.h"
#include "IntentRecord.h"
#include "Decision.h"

/**
 * In-memory CSSV constitutional ledger for Unreal host.
 * Mirrors engine/cssv/CssvRegistry.js and Unity CssvRegistry.cs.
 */
class GOVERNEDENGINE_API FCssvRegistry
{
public:
	static FCssvRegistry& Get();

	void RegisterArtifact(const FString& Id, const FString& ArtifactType, const TSharedPtr<FJsonObject>& Payload = nullptr);
	void RegisterTransition(
		const FString& Id,
		const FIntentRecord& Intent,
		const FEvidenceBundle& Evidence,
		const FDecision& Decision,
		const FString& Authority,
		double TimeSeconds = 0.0);
	void RegisterFrame(const FFrameProvenance& Frame);

	FString ExportJson() const;
	int32 GetFrameCount() const { return Frames.Num(); }
	int32 GetTransitionCount() const { return Transitions.Num(); }

private:
	FCssvRegistry() = default;

	FString HostId = TEXT("unreal");
	FString StateId = TEXT("state-0000");
	int32 FrameIndex = 0;

	TArray<TSharedPtr<FJsonValue>> Artifacts;
	TArray<TSharedPtr<FJsonValue>> Transitions;
	TArray<TSharedPtr<FJsonValue>> Frames;
};
