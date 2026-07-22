#include "FrameProvenance.h"

TArray<FFrameProvenance> FProvenanceRecorder::Frames;

void FProvenanceRecorder::Record(const FFrameProvenance& Frame)
{
	Frames.Add(Frame);
}

const TArray<FFrameProvenance>& FProvenanceRecorder::GetFrames()
{
	return Frames;
}

void FProvenanceRecorder::Clear()
{
	Frames.Reset();
}
