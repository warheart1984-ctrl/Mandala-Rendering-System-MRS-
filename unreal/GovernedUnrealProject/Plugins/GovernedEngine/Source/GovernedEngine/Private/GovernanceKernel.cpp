#include "GovernanceKernel.h"
#include "Misc/Guid.h"

namespace
{
	bool IsMutationType(const FString& Type)
	{
		return Type == TEXT("update_world") ||
			Type == TEXT("play_timeline") ||
			Type == TEXT("render_4d_tesseract") ||
			Type == TEXT("artifact.picture") ||
			Type == TEXT("artifact.movie") ||
			Type == TEXT("render.session");
	}

	bool EvalTimelineCondition(const FString& Condition, const FString& TimelineId, double DriftScore)
	{
		TArray<FString> Parts;
		Condition.ParseIntoArray(Parts, TEXT("&&"), true);
		for (FString Part : Parts)
		{
			Part.TrimStartAndEndInline();
			if (Part.StartsWith(TEXT("intent.timeline ==")))
			{
				int32 Q1 = Part.Find(TEXT("'"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
				int32 Q2 = Part.Find(TEXT("'"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				if (Q1 == INDEX_NONE || Q2 <= Q1) return false;
				const FString Expected = Part.Mid(Q1 + 1, Q2 - Q1 - 1);
				if (TimelineId != Expected) return false;
				continue;
			}
			if (Part.StartsWith(TEXT("drift_score >")))
			{
				const FString NumStr = Part.Mid(13).TrimStartAndEnd();
				const double Thr = FCString::Atod(*NumStr);
				if (!(DriftScore > Thr)) return false;
				continue;
			}
			return false;
		}
		return true;
	}

	bool HasAllEvidence(const FEvidenceBundle& Evidence, const TArray<FString>& Require)
	{
		if (Evidence.bEmpty && Require.Num() > 0) return false;
		TSet<FString> Ids;
		if (const FString* EvidenceId = Evidence.Fields.Find(TEXT("evidenceId")))
		{
			Ids.Add(*EvidenceId);
		}
		if (const FString* EvidenceIds = Evidence.Fields.Find(TEXT("evidenceIds")))
		{
			TArray<FString> Parts;
			EvidenceIds->ParseIntoArray(Parts, TEXT(","), true);
			for (FString Part : Parts)
			{
				Part.TrimStartAndEndInline();
				if (!Part.IsEmpty()) Ids.Add(Part);
			}
		}
		for (const FString& Req : Require)
		{
			if (!Ids.Contains(Req)) return false;
		}
		return true;
	}

	double EvalModifier(const FString& Modifier, const FString& Param, double Current)
	{
		const int32 Star = Modifier.Find(TEXT("*"));
		if (Star > 0)
		{
			const FString Left = Modifier.Left(Star).TrimStartAndEnd();
			const FString Right = Modifier.Mid(Star + 1).TrimStartAndEnd();
			if (Left == Param || Left == TEXT("speed"))
			{
				return Current * FCString::Atod(*Right);
			}
		}
		return Current;
	}

	double GetDriftScore(const FEvidenceBundle& Evidence)
	{
		if (const FString* Drift = Evidence.Fields.Find(TEXT("driftScore")))
		{
			return FCString::Atod(**Drift);
		}
		return 0.0;
	}
}

FDecision FDecisionEngine::Resolve(const FIntentRecord& Intent, const FEvidenceBundle& Evidence, const FPolicySet& Policies)
{
	FDecision D;
	D.DecisionId = FString::Printf(TEXT("decision-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));

	if (Intent.Id.IsEmpty() && Intent.Type.IsEmpty())
	{
		D.bAllowed = false;
		D.Verdict = TEXT("deny");
		D.Reason = TEXT("No execution without intent.");
		D.Violations.Add(TEXT("policy-no-execution-without-intent"));
		return D;
	}

	const FString Type = Intent.Type.IsEmpty() ? Intent.Kind : Intent.Type;
	const FString TimelineId = Intent.TimelineId;
	const double DriftScore = GetDriftScore(Evidence);
	TMap<FString, float> ParamAdjust;

	for (const FPolicy& Policy : Policies.Policies)
	{
		if (Policy.Condition == TEXT("intent != null"))
		{
			continue;
		}

		if (Policy.Condition == TEXT("require_evidence_for_mutation"))
		{
			if (IsMutationType(Type) && Evidence.bEmpty && Intent.EvidenceId.IsEmpty())
			{
				D.Violations.Add(Policy.Id);
			}
		}

		if (Policy.Condition == TEXT("play_timeline_or_render_4d"))
		{
			if (Type == TEXT("play_timeline") || Type == TEXT("render_4d_tesseract"))
			{
				if (Policy.Rule == TEXT("attach_provenance"))
				{
					D.bAttachProvenance = true;
				}
				if (Evidence.bEmpty && Intent.EvidenceId.IsEmpty())
				{
					D.Violations.Add(Policy.Id);
				}
			}
		}

		if (Policy.Condition == TEXT("actor_has_contract"))
		{
			if (Intent.Actor.IsEmpty())
			{
				D.Violations.Add(Policy.Id);
			}
		}

		if (Policy.Condition == TEXT("play_timeline_requires_world") ||
			Policy.Id == TEXT("policy-play-timeline-requires-world"))
		{
			if (Type == TEXT("play_timeline") && Intent.World.IsEmpty())
			{
				D.Violations.Add(Policy.Id);
			}
		}

		if (!Policy.Condition.IsEmpty() && Policy.Condition.Contains(TEXT("intent.timeline ==")))
		{
			if (EvalTimelineCondition(Policy.Condition, TimelineId, DriftScore))
			{
				if (Policy.Rule == TEXT("deny_if_false") && Policy.Require.Num() > 0)
				{
					if (!HasAllEvidence(Evidence, Policy.Require))
					{
						D.Violations.Add(Policy.Id);
					}
				}
				if (Policy.Rule == TEXT("modify_param") && !Policy.Param.IsEmpty() && !Policy.Modifier.IsEmpty())
				{
					double Current = 1.0;
					const double Modified = EvalModifier(Policy.Modifier, Policy.Param, Current);
					ParamAdjust.Add(Policy.Param, static_cast<float>(Modified));
				}
			}
		}
	}

	if (D.Violations.Num() > 0)
	{
		D.bAllowed = false;
		D.Verdict = TEXT("deny");
		D.Reason = TEXT("Constitutional policy violation");
		return D;
	}

	D.bAllowed = true;
	D.Verdict = TEXT("allow");
	D.Reason = TEXT("Policies satisfied");
	D.ParamAdjust = ParamAdjust;
	return D;
}

FGovernanceKernel::FGovernanceKernel() = default;

FDecision FGovernanceKernel::EvaluateIntent(const FIntentRecord& Intent, const FEvidenceBundle& Evidence)
{
	const FWorldId World(Intent.World);
	const FPolicySet Policies = CKL.GetPoliciesForWorld(World);
	FDecision Decision = FDecisionEngine::Resolve(Intent, Evidence, Policies);
	CKL.RecordPrecedent(Intent, Decision);
	return Decision;
}
