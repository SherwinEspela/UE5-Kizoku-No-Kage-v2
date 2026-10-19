#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FAnimStitchPreview;

/** Read-only Details panel for the selected bone: live translation / rotation / scale at the current scrub time. */
class SAnimStitchBoneDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchBoneDetails) {}
		SLATE_ARGUMENT(TSharedPtr<FAnimStitchPreview>, Preview)
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
	TSharedRef<SWidget> MakeGroup(const FText& Title, ESpace Space);

	TSharedPtr<FAnimStitchPreview> Preview;
};
