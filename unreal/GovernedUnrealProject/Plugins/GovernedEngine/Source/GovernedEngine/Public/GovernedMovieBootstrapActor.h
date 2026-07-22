#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GovernedMovieBootstrapActor.generated.h"

/**
 * BeginPlay / console: submit governed artifact.movie intent.
 * Requires UGovernedMovieCaptureComponent in the level.
 */
UCLASS()
class GOVERNEDENGINE_API AGovernedMovieBootstrapActor : public AActor
{
	GENERATED_BODY()

public:
	AGovernedMovieBootstrapActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	float Seconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	int32 Fps = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	FString WorldId = TEXT("world-mythar-plains");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	FString TimelineId = TEXT("opening_4d_reveal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Movie")
	bool bExecuteOnBeginPlay = false;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Governed|Movie")
	void MakeGovernedMovie();
};
