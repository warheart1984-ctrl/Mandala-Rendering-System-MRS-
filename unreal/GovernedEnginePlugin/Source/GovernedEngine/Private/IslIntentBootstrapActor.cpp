#include "IslIntentBootstrapActor.h"
#include "FIslEngine.h"
#include "ExecutionOrchestrator.h"
#include "CssvRegistry.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"

AIslIntentBootstrapActor::AIslIntentBootstrapActor()
{
	PrimaryActorTick.bCanEverTick = false;
	IslScriptPath = FPaths::Combine(
		FPaths::ProjectPluginsDir(),
		TEXT("GovernedEngine/Content/Scripts/Opening4DReveal.isl"));
	ContextJson = TEXT("{\"actor\":\"4dce.isl\"}");
}

void AIslIntentBootstrapActor::BeginPlay()
{
	Super::BeginPlay();

	TSharedPtr<FJsonObject> WorldPayload = MakeShared<FJsonObject>();
	WorldPayload->SetStringField(TEXT("worldId"), TEXT("world-mythar-plains"));
	FCssvRegistry::Get().RegisterArtifact(TEXT("world-mythar-plains"), TEXT("world"), WorldPayload);

	TSharedPtr<FJsonObject> TimelinePayload = MakeShared<FJsonObject>();
	TimelinePayload->SetStringField(TEXT("timelineId"), TEXT("opening_4d_reveal"));
	FCssvRegistry::Get().RegisterArtifact(TEXT("opening_4d_reveal"), TEXT("timeline"), TimelinePayload);

	FString IslSource;
	if (!FFileHelper::LoadFileToString(IslSource, *IslScriptPath))
	{
		UE_LOG(LogTemp, Error, TEXT("IslIntentBootstrap: failed to load %s"), *IslScriptPath);
		return;
	}

	const FIntentRecord Intent = FIslEngine::CompileAndEvaluate(IslSource, ContextJson);
	FExecutionOrchestrator::Execute(Intent);
}
