#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "AnimStitchTimeline.h"

class UAnimStitchTimelineAsset;

/** Foot lock settings popup (foot bones, ground height, plant thresholds). Edits go straight to the timeline asset, undoable. */
class SAnimStitchFootLockSettings : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchFootLockSettings) {}
		SLATE_ARGUMENT(TObjectPtr<UAnimStitchTimelineAsset>, Timeline)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchFootLockSettings"); }

private:
	void Apply(TFunctionRef<void(FAnimStitchFootLockSettings&)> Edit, const FText& Label);
	const FAnimStitchFootLockSettings& Current() const;
	/** Rebuilds the chips of the extra ground contacts (paws, other end effectors beyond the left/right pair). */
	void RebuildContactChips();

	TObjectPtr<UAnimStitchTimelineAsset> Timeline;
	TArray<TSharedPtr<FString>> BoneItems;
	TSharedPtr<FString> LeftSelected;
	TSharedPtr<FString> RightSelected;
	TSharedPtr<class SWrapBox> ContactChips;
};
