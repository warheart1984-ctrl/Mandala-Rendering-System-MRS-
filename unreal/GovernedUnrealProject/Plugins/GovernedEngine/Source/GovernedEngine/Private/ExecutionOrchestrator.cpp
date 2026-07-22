#include "ExecutionOrchestrator.h"
#include "GovernanceKernel.h"
#include "Decision.h"
#include "CKL.h"
#include "TimelineExecutor.h"
#include "CssvRegistry.h"
#include "GovernedMovieCapture.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FGovernanceKernel& KernelInstance()
	{
		static FGovernanceKernel Kernel;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			FString Json;
			const TArray<FString> PolicyCandidates = {
				FPaths::Combine(
					FPaths::ProjectPluginsDir(),
					TEXT("GovernedEngine/Content/Governed/default.policies.json")),
				FPaths::Combine(
					FPaths::ProjectContentDir(),
					TEXT("Governed/default.policies.json")),
				FPaths::Combine(
					FPaths::ProjectDir(),
					TEXT("../engine/governance/policies/default.policies.json")),
			};
			for (const FString& PolicyPath : PolicyCandidates)
			{
				if (FFileHelper::LoadFileToString(Json, *PolicyPath))
				{
					break;
				}
			}
			if (!Json.IsEmpty())
			{
				Kernel.GetCKL().LoadPoliciesFromJson(Json);
			}
			else
			{
				const FString Embedded = TEXT(R"([
				  {"id":"policy-play-timeline-requires-world","scope":"render","condition":"play_timeline_requires_world","rule":"deny_if_false","severity":"critical","message":"play_timeline requires world"}
				])");
				Kernel.GetCKL().LoadPoliciesFromJson(Embedded);
			}
			bLoaded = true;
		}
		return Kernel;
	}

	bool IsMovieIntent(const FString& Type)
	{
		return Type == TEXT("artifact.movie") ||
			Type == TEXT("artifact.movie.export") ||
			Type == TEXT("render.session") ||
			Type == TEXT("render.session.start");
	}

	void ExecuteInternal(const FIntentRecord& Intent, FEvidenceBundle Evidence)
	{
		if (!Intent.EvidenceId.IsEmpty() && Evidence.bEmpty)
		{
			Evidence.bEmpty = false;
			Evidence.Fields.Add(TEXT("evidenceId"), Intent.EvidenceId);
		}

		const FDecision Decision = KernelInstance().EvaluateIntent(Intent, Evidence);

		FCssvRegistry::Get().RegisterTransition(
			FString::Printf(TEXT("tx-%s"), *Intent.Id),
			Intent,
			Evidence,
			Decision,
			Intent.Actor.IsEmpty() ? TEXT("runtime.unreal") : Intent.Actor);

		if (!Decision.bAllowed)
		{
			UE_LOG(LogTemp, Warning, TEXT("Governed execute DENIED: %s (%s)"), *Decision.Reason, *Intent.Id);
			return;
		}

		const FString Type = Intent.Type.IsEmpty() ? Intent.Kind : Intent.Type;
		if (IsMovieIntent(Type))
		{
			UE_LOG(LogTemp, Log, TEXT("Governed execute ALLOWED movie: %s"), *Intent.Id);
			FGovernedMovieCapture::TryStart(Intent, Decision, Evidence);
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Governed execute ALLOWED: %s → timeline %s"), *Intent.Id, *Intent.TimelineId);
		FTimelineExecutor::Play(Intent);
	}
}

void FExecutionOrchestrator::Execute(const FIntentRecord& Intent)
{
	FEvidenceBundle Evidence = FEvidenceBundle::Empty();
	if (!Intent.EvidenceId.IsEmpty())
	{
		Evidence.bEmpty = false;
		Evidence.Fields.Add(TEXT("evidenceId"), Intent.EvidenceId);
	}
	ExecuteInternal(Intent, Evidence);
}

void FExecutionOrchestrator::ExecuteMovie(const FIntentRecord& Intent, float Seconds, int32 Fps)
{
	FEvidenceBundle Evidence = FEvidenceBundle::Empty();
	Evidence.bEmpty = false;
	Evidence.Timestamp = FDateTime::UtcNow().ToIso8601();
	Evidence.Fields.Add(TEXT("evidenceId"), Intent.EvidenceId.IsEmpty() ? TEXT("ev-unreal-movie-001") : Intent.EvidenceId);
	Evidence.Fields.Add(TEXT("evidenceIds"), Intent.EvidenceId.IsEmpty() ? TEXT("ev-unreal-movie-001") : Intent.EvidenceId);
	Evidence.Fields.Add(TEXT("seconds"), FString::SanitizeFloat(Seconds));
	Evidence.Fields.Add(TEXT("fps"), FString::FromInt(Fps));
	ExecuteInternal(Intent, Evidence);
}
