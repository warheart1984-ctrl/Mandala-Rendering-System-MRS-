#include "GovernedEngine.h"
#include "Modules/ModuleManager.h"
#include "UnrealRuntimeAdapter.h"
#include "ConformanceChecker.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"
#include "GovernedMovieBootstrapActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FGovernedEngineModule"

namespace
{
	void RunConformanceChecks()
	{
		FString ProfileJson;
		const FString ProfilePath = FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("../engine/conformance/default.conformance-profile.json"));
		if (!FFileHelper::LoadFileToString(ProfileJson, *ProfilePath))
		{
			UE_LOG(LogTemp, Error, TEXT("[Conformance] Profile not found: %s"), *ProfilePath);
			return;
		}

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ProfileJson);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[Conformance] Failed to parse profile JSON"));
			return;
		}

		TArray<FConformanceCheckDef> Checks;
		const TArray<TSharedPtr<FJsonValue>>* ChecksArr = nullptr;
		if (Root->TryGetArrayField(TEXT("checks"), ChecksArr))
		{
			for (const TSharedPtr<FJsonValue>& V : *ChecksArr)
			{
				const TSharedPtr<FJsonObject> Obj = V->AsObject();
				if (!Obj.IsValid()) continue;
				FConformanceCheckDef Def;
				Def.Id = Obj->GetStringField(TEXT("id"));
				Def.Domain = Obj->GetStringField(TEXT("domain"));
				Def.Description = Obj->GetStringField(TEXT("description"));
				Def.Severity = Obj->GetStringField(TEXT("severity"));
				Checks.Add(Def);
			}
		}

		const FPolicySet Policies = FUnrealRuntimeAdapter::LoadDefaultPolicies();
		const FRuntimeAdapter Adapter = FUnrealRuntimeAdapter::Build(Policies);
		const FConformanceReport Report = FConformanceChecker::Evaluate(TEXT("unreal"), Checks, Adapter);

		UE_LOG(LogTemp, Log, TEXT("[Conformance] Runtime: %s — %d/%d passed (compliant=%s)"),
			*Report.Runtime, Report.Passed, Report.Total,
			Report.bCompliant ? TEXT("yes") : TEXT("no"));

		for (const FCheckResult& R : Report.Results)
		{
			UE_LOG(LogTemp, Log, TEXT("  [%s] %s%s"),
				R.bPass ? TEXT("PASS") : TEXT("FAIL"),
				*R.Id,
				R.Reason.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" — %s"), *R.Reason));
		}
	}

	void RunMakeMovie()
	{
		UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
		if (!World && GEngine)
		{
			for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
			{
				if (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game)
				{
					World = Ctx.World();
					break;
				}
			}
		}
		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("[Movie] No play world — enter PIE first"));
			return;
		}
		for (TActorIterator<AGovernedMovieBootstrapActor> It(World); It; ++It)
		{
			(*It)->MakeGovernedMovie();
			return;
		}
		// Fallback: synthesize intent without actor present
		AGovernedMovieBootstrapActor* Temp = World->SpawnActor<AGovernedMovieBootstrapActor>();
		if (Temp)
		{
			Temp->MakeGovernedMovie();
			Temp->Destroy();
		}
	}
}

void FGovernedEngineModule::StartupModule()
{
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("GovernedEngine.RunConformance"),
		TEXT("Run constitutional conformance probes against the Unreal host"),
		FConsoleCommandDelegate::CreateStatic(&RunConformanceChecks),
		ECVF_Default);
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("GovernedEngine.MakeMovie"),
		TEXT("Submit governed artifact.movie intent (requires UGovernedMovieCaptureComponent in level)"),
		FConsoleCommandDelegate::CreateStatic(&RunMakeMovie),
		ECVF_Default);
}

void FGovernedEngineModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGovernedEngineModule, GovernedEngine)
