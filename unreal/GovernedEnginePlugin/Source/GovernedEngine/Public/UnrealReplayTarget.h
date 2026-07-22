#pragma once

#include "CoreMinimal.h"
#include "ReplayService.h"
#include "FourDRendererComponent.h"

/** Unreal IReplayTarget — re-applies FrameProvenance to a 4D renderer. Status: skeleton. */
class FUnrealReplayTarget : public IReplayTarget
{
public:
	UFourDRendererComponent* Renderer = nullptr;

	virtual void ApplyFrame(const FFrameProvenance& Frame) override
	{
		if (!Renderer) return;
		if (const double* Speed = Frame.Parameters.Find(TEXT("speed")))
			Renderer->Speed = static_cast<float>(*Speed);
		if (const double* D4 = Frame.Parameters.Find(TEXT("d4")))
			Renderer->D4 = static_cast<float>(*D4);
		if (const double* D3 = Frame.Parameters.Find(TEXT("d3")))
			Renderer->D3 = static_cast<float>(*D3);
	}
};
