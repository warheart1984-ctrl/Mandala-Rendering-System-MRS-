#include "GovernedMovieBootstrapActor.h"
#include "ExecutionOrchestrator.h"
#include "IntentRecord.h"
#include "Misc/Guid.h"

AGovernedMovieBootstrapActor::AGovernedMovieBootstrapActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGovernedMovieBootstrapActor::BeginPlay()
{
	Super::BeginPlay();
	if (bExecuteOnBeginPlay)
	{
		MakeGovernedMovie();
	}
}

void AGovernedMovieBootstrapActor::MakeGovernedMovie()
{
	FIntentRecord Intent;
	Intent.Id = FString::Printf(TEXT("movie-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
	Intent.Type = TEXT("artifact.movie");
	Intent.Kind = TEXT("artifact.movie");
	Intent.Actor = TEXT("runtime.unreal");
	Intent.World = WorldId;
	Intent.TimelineId = TimelineId;
	Intent.EvidenceId = TEXT("ev-unreal-movie-001");
	Intent.Goal = FString::Printf(TEXT("Record governed %.0fs PNG sequence"), Seconds);
	FExecutionOrchestrator::ExecuteMovie(Intent, Seconds, Fps);
}
