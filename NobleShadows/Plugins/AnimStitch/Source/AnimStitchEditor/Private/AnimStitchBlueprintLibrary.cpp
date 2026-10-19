#include "AnimStitchBlueprintLibrary.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchBaker.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

UAnimSequence* UAnimStitchBlueprintLibrary::BakeTimeline(UAnimStitchTimelineAsset* Timeline, const FString& OutputAssetPath, int32 FrameRate, bool bOverwriteExisting, FString& OutError)
{
	OutError.Reset();
	if (!Timeline)
	{
		OutError = LOCTEXT("ErrNoTimeline", "No timeline asset was given.").ToString();
		return nullptr;
	}
	FAnimStitchBakeOptions Options;
	Options.FrameRate = FrameRate > 0 ? FFrameRate(FrameRate, 1) : FFrameRate(0, 1);
	Options.bOverwriteExisting = bOverwriteExisting;
	return FAnimStitchBaker::Bake(Timeline->Timeline, OutputAssetPath, Options, OutError);
}

#undef LOCTEXT_NAMESPACE

