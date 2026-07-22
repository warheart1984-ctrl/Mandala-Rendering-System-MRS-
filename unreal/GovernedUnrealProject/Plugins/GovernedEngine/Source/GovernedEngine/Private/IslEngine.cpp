#include "FIslEngine.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "Internationalization/Regex.h"

namespace
{
	FString MatchGroup(const FString& Source, const FString& Pattern, int32 GroupIndex = 1)
	{
		FRegexPattern P(Pattern);
		FRegexMatcher M(P, Source);
		if (M.FindNext())
		{
			return M.GetCaptureGroup(GroupIndex);
		}
		return FString();
	}

	FString StripComments(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines);
		FString Out;
		for (FString Line : Lines)
		{
			const int32 Comment = Line.Find(TEXT("//"));
			if (Comment != INDEX_NONE)
			{
				Line = Line.Left(Comment);
			}
			Out += Line;
			Out += TEXT("\n");
		}
		return Out;
	}

	FString MatchContextField(const FString& Json, const FString& Key)
	{
		const FString Pattern = FString::Printf(TEXT("\"%s\"\\s*:\\s*\"([^\"]*)\""), *Key);
		return MatchGroup(Json, Pattern);
	}
}

FIntentRecord FIslEngine::CompileAndEvaluate(const FString& Source, const FString& ContextJson)
{
	const FString Clean = StripComments(Source).TrimStartAndEnd();
	FIntentRecord Intent;
	Intent.Id = FString::Printf(TEXT("intent-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	Intent.Timestamp = FDateTime::UtcNow().ToIso8601();
	Intent.Source = TEXT("isl-v2");
	Intent.Actor = MatchContextField(ContextJson, TEXT("actor"));
	if (Intent.Actor.IsEmpty())
	{
		Intent.Actor = TEXT("4dce.isl");
	}

	const FString Verb = MatchGroup(Clean, TEXT("intent\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\("));
	Intent.Type = Verb;
	Intent.Kind = Verb;
	Intent.Goal = FString::Printf(TEXT("ISL %s"), *Verb);

	const FString Arg0 = MatchGroup(Clean, TEXT("\\(\\s*\"([^\"]*)\""));
	const FString World = MatchGroup(Clean, TEXT("world\\s*\\(\\s*\"([^\"]*)\"\\s*\\)"));
	Intent.World = World;

	if (Verb == TEXT("play_timeline"))
	{
		Intent.TimelineId = Arg0;
	}
	else if (Verb == TEXT("render_4d_tesseract"))
	{
		Intent.Entity = Arg0;
	}

	const FString At = MatchGroup(Clean, TEXT("at\\s+\"([^\"]*)\""));
	if (!At.IsEmpty())
	{
		Intent.At = At;
	}

	const FString Evidence = MatchGroup(Clean, TEXT("evidence\\s*\\(\\s*\"([^\"]*)\"\\s*\\)"));
	if (!Evidence.IsEmpty())
	{
		Intent.EvidenceId = Evidence;
	}

	if (Verb.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ISL: no intent verb found"));
	}

	return Intent;
}
