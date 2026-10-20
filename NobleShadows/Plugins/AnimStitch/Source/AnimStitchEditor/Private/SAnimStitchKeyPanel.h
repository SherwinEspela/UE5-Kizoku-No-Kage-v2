#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AnimStitchTimeline.h"

class SAnimStitchTimeline;
class FScopedTransaction;

/**
 * Numeric entry for one bone-track key (Sequencer style): translate, rotate, scale / pole, weight.
 * Lives under the expanded lane of the timeline. Shows the selected key of that lane, or the value at the playhead when no key is selected
 * (editing then keys the playhead).
 */
class SAnimStitchKeyPanel : public SCompoundWidget
{
public:
	/** For IK tracks: the limb as it looks right now (effector, pole), used while the key has no target of its own. */
	using FLiveIKDelegate = TFunction<bool(int32 TrackIndex, FAnimStitchTrackKey& OutKey)>;

	SLATE_BEGIN_ARGS(SAnimStitchKeyPanel) {}
		SLATE_ARGUMENT(TSharedPtr<SAnimStitchTimeline>, Timeline)
		/** The track this panel edits (a fixed lane, or follows the selected control in the Details tab). */
		SLATE_ATTRIBUTE(int32, TrackIndex)
		SLATE_ARGUMENT(FLiveIKDelegate, LiveIK)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	const FAnimStitchTrack* GetTrack() const;
	FAnimStitchTrackKey GetKey() const;
	void Edit(TFunctionRef<void(FAnimStitchTrackKey&)> Mutator);
	TSharedRef<SWidget> Vec3Row(const FText& Label, TFunction<FVector(const FAnimStitchTrackKey&)> Get, TFunction<void(FAnimStitchTrackKey&, const FVector&)> Set, TAttribute<EVisibility> Visible, double Delta);

	TSharedPtr<SAnimStitchTimeline> Timeline;
	FLiveIKDelegate LiveIK;
	TAttribute<int32> TrackIndexAttr;
	TSharedPtr<FScopedTransaction> SliderTransaction;
};
