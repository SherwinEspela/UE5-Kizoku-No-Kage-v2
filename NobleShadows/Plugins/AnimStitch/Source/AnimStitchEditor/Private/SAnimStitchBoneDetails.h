#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FAnimStitchPreview;
class SAnimStitchTimeline;

/**
 * Details panel: the selected control's transform (and the IK key's weight), editable like dragging the control, above the read-only live
 * translation / rotation / scale of the selected bone at the current scrub time.
 */
class SAnimStitchBoneDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchBoneDetails) {}
		SLATE_ARGUMENT(TSharedPtr<FAnimStitchPreview>, Preview)
		SLATE_ARGUMENT(TSharedPtr<SAnimStitchTimeline>, Timeline)
		SLATE_ARGUMENT(TFunction<bool(int32, struct FAnimStitchTrackKey&)>, LiveIK)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum class ESpace : uint8 { Local, Component };
	enum class EPart : uint8 { Translation, Rotation, Scale };

	FText GetValue(ESpace Space, EPart Part, int32 Axis) const;
	FText GetHeader() const;
	FText GetSubHeader() const;
	EVisibility GetNoSelectionVisibility() const;
	EVisibility GetSelectionVisibility() const;
	EVisibility GetControlVisibility() const;
	bool GetSelectedControlTrack(int32& OutTrack) const;
	FText GetControlTitle() const;
	TSharedRef<SWidget> MakeGroup(const FText& Title, ESpace Space);

	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<SAnimStitchTimeline> Timeline;
};
