#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IslIntentBootstrapActor.generated.h"

/**
 * Place in level. BeginPlay: ISL → IntentRecord → ExecutionOrchestrator → Timeline → 4D.
 * Status: skeleton until plugin is compiled in Unreal Editor.
 */
UCLASS()
class GOVERNEDENGINE_API AIslIntentBootstrapActor : public AActor
{
	GENERATED_BODY()

public:
	AIslIntentBootstrapActor();

	/** Absolute or project-relative path to Opening4DReveal.isl */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|ISL")
	FString IslScriptPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|ISL")
	FString ContextJson;

protected:
	virtual void BeginPlay() override;
};
