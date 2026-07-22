#include "GovernedEngineEditor.h"
#include "GovernedMovieRenderQueue.h"
#include "GovernanceKernel.h"
#include "Decision.h"
#include "CKL.h"
#include "IntentRecord.h"
#include "CssvRegistry.h"
#include "Misc/Guid.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FGovernedEngineEditorModule"

namespace
{
	FGovernanceKernel& EditorKernel()
	{
		static FGovernanceKernel Kernel;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			FString Json;
			const FString PolicyPath = FPaths::Combine(
				FPaths::ProjectPluginsDir(),
				TEXT("GovernedEngine/Content/Governed/default.policies.json"));
			if (FFileHelper::LoadFileToString(Json, *PolicyPath))
			{
				Kernel.GetCKL().LoadPoliciesFromJson(Json);
			}
			bLoaded = true;
		}
		return Kernel;
	}

	void RunMakeMovieMrq()
	{
		FIntentRecord Intent;
		Intent.Id = FString::Printf(TEXT("movie-mrq-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
		Intent.Type = TEXT("artifact.movie");
		Intent.Kind = TEXT("artifact.movie");
		Intent.Actor = TEXT("runtime.unreal");
		Intent.World = TEXT("world-mythar-plains");
		Intent.TimelineId = TEXT("opening_4d_reveal");
		Intent.EvidenceId = TEXT("ev-unreal-movie-001");
		Intent.Goal = TEXT("Record governed ProRes via Movie Render Queue");

		FEvidenceBundle Evidence = FEvidenceBundle::Empty();
		Evidence.bEmpty = false;
		Evidence.Fields.Add(TEXT("evidenceId"), Intent.EvidenceId);
		Evidence.Fields.Add(TEXT("seconds"), TEXT("8"));
		Evidence.Fields.Add(TEXT("fps"), TEXT("30"));

		const FDecision Decision = EditorKernel().EvaluateIntent(Intent, Evidence);
		FCssvRegistry::Get().RegisterTransition(
			FString::Printf(TEXT("tx-%s"), *Intent.Id),
			Intent,
			Evidence,
			Decision,
			Intent.Actor);

		if (!Decision.bAllowed)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MRQ] CKL denied: %s"), *Decision.Reason);
			return;
		}

		FString ContainerPath;
		FString SessionDir;
		if (!FGovernedMovieRenderQueue::StartProResCapture(Intent, Decision, 8.f, 30, ContainerPath, SessionDir))
		{
			UE_LOG(LogTemp, Warning, TEXT("[MRQ] Failed — fall back to GovernedEngine.MakeMovie (PNG) in PIE"));
		}
	}
}

void FGovernedEngineEditorModule::StartupModule()
{
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("GovernedEngine.MakeMovieMRQ"),
		TEXT("Governed artifact.movie via Movie Render Queue (Apple ProRes .mov)"),
		FConsoleCommandDelegate::CreateStatic(&RunMakeMovieMrq),
		ECVF_Default);
}

void FGovernedEngineEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGovernedEngineEditorModule, GovernedEngineEditor)
