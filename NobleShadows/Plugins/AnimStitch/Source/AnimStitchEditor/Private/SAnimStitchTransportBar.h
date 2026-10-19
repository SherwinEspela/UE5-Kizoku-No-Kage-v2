#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

class SAnimStitchTimeline;

/** The scrollbar under the timeline: the thumb is the visible time range; drag its body to pan, its ends to zoom. */
class SAnimStitchRangeBar : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchRangeBar) {}
		SLATE_ARGUMENT(TSharedPtr<SAnimStitchTimeline>, Timeline)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100.0, 18.0); }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	enum class EGrab : uint8 { None, Left, Right, Body };
	void GetBarRange(double& OutStart, double& OutEnd) const;
	EGrab HitThumb(float X, float Width) const;

	TSharedPtr<SAnimStitchTimeline> Timeline;
	EGrab Grab = EGrab::None;
	double GrabStart = 0.0, GrabEnd = 0.0, GrabRangeStart = 0.0, GrabRangeEnd = 1.0;
	float GrabX = 0.f;
};

/**
 * Bottom bar of the timeline, mirroring Sequencer's (without record and marked frames): transport buttons, loop, time / frame readout,
 * frames-or-seconds toggle and the pan / zoom range bar.
 */
class SAnimStitchTransportBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchTransportBar) {}
		SLATE_ARGUMENT(TSharedPtr<SAnimStitchTimeline>, Timeline)
		SLATE_ARGUMENT(TFunction<void()>, OnGoToStart)
		SLATE_ARGUMENT(TFunction<void()>, OnPreviousKey)
		SLATE_ARGUMENT(TFunction<void()>, OnPlayPause)
		SLATE_ARGUMENT(TFunction<void()>, OnNextKey)
		SLATE_ARGUMENT(TFunction<void()>, OnGoToEnd)
		SLATE_ARGUMENT(TFunction<void()>, OnStop)
		SLATE_ARGUMENT(TFunction<void()>, OnToggleLoop)
		SLATE_ARGUMENT(TFunction<void(double)>, OnSetPlayhead)
		SLATE_ARGUMENT(TFunction<bool()>, IsPlaying)
		SLATE_ARGUMENT(TFunction<bool()>, IsLooping)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FText FormatTime(double Seconds) const;

	TSharedPtr<SAnimStitchTimeline> Timeline;
};
