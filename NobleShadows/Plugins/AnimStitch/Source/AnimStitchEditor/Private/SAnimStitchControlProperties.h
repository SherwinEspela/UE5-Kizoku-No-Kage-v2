#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AnimStitchTimeline.h"

class FAnimStitchPreview;
class SAnimStitchTimeline;
class UAnimStitchTimelineAsset;

/**
 * "Control Properties" tab: the look of the selected control shape on its own. Per-control values override the global
 * Show > Controls values: opacity, size, shape (box / sphere) and a display offset to separate overlapping shapes.
 */
class SAnimStitchControlProperties : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchControlProperties) {}
		SLATE_ARGUMENT(TSharedPtr<FAnimStitchPreview>, Preview)
		SLATE_ARGUMENT(TSharedPtr<SAnimStitchTimeline>, Timeline)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	bool GetTarget(UAnimStitchTimelineAsset*& OutAsset, int32& OutTrack, bool& bOutPole) const;
	FAnimStitchControlStyle GetStyle() const;
	void Edit(TFunctionRef<void(FAnimStitchControlStyle&)> Mutator);
	TSharedRef<SWidget> MakeOffsetRow(const FText& Label, int32 Axis);

	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<SAnimStitchTimeline> Timeline;
	TSharedPtr<class FScopedTransaction> SliderTransaction;
};
