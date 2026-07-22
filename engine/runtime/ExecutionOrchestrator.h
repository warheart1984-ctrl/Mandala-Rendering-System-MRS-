#pragma once



#ifndef GOVERNEDENGINE_API

#define GOVERNEDENGINE_API

#endif



#include "CoreMinimal.h"

#include "IntentRecord.h"



class GOVERNEDENGINE_API FExecutionOrchestrator

{

public:

	static void Execute(const FIntentRecord& Intent);



	/** Governed movie export with seconds/fps evidence fields. */

	static void ExecuteMovie(const FIntentRecord& Intent, float Seconds, int32 Fps);

};


