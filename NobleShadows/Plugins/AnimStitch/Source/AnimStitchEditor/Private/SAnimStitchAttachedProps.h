#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FAnimStitchPreview;
class SAnimStitchTimeline;

/**
 * "Attached Props" tab: every prop attached to the preview character, with the socket it sits on, a visibility toggle and a remove
 * button. Clicking a row highlights the prop in the viewport. Everything lives in the timeline asset (preview only, never baked).
 */
class SAnimStitchAttachedProps : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchAttachedProps) {}
		SLATE_ARGUMENT(TSharedPtr<FAnimStitchPreview>, Preview)
		SLATE_ARGUMENT(TSharedPtr<SAnimStitchTimeline>, Timeline)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void Rebuild();

	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<SAnimStitchTimeline> Timeline;
	TArray<TSharedPtr<int32>> Items;
	TSharedPtr<SListView<TSharedPtr<int32>>> List;
	uint32 LastEditCounter = MAX_uint32;
	uint32 LastManualCounter = MAX_uint32;
	TWeakObjectPtr<UObject> LastAsset;
};
