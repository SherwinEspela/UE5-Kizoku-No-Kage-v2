#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "EditorUndoClient.h"
#include "AnimStitchTimelineAsset.h"

class FScopedTransaction;

DECLARE_DELEGATE_OneParam(FAnimStitchWarningsDelegate, const TArray<FText>&);

/** Custom Slate NLE-style timeline: ruler, a single clip track, overlaps, trim handles, drag & drop. */
class SAnimStitchTimeline : public SLeafWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchTimeline) {}
		SLATE_ARGUMENT(TObjectPtr<UAnimStitchTimelineAsset>, TimelineAsset)
		SLATE_EVENT(FSimpleDelegate, OnSelectionChanged)
		SLATE_EVENT(FSimpleDelegate, OnPlayheadChanged)
		SLATE_EVENT(FAnimStitchWarningsDelegate, OnWarnings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAnimStitchTimeline() override;

	// Selection (an overlap selection refers to the crossfade INTO clip SelectedClip).
	int32 GetSelectedClip() const { return SelectedClip; }
	bool IsOverlapSelected() const { return bOverlapSelected; }

	void DeleteSelected();
	void DuplicateSelected();
	void SetSelectedBlendCurve(EAnimStitchBlendCurve Curve);
	void ZoomToFit();

	// Cut commands act on the playhead: the selected clip if the playhead is inside it, else the topmost clip there.
	bool CanEditAtPlayhead() const { return FindCutTarget() != INDEX_NONE; }
	void SplitAtPlayhead();
	void TrimStartAtPlayhead();
	void TrimEndAtPlayhead();

	double GetPlayhead() const { return Playhead; }
	void SetPlayhead(double Time) { Playhead = FMath::Clamp(Time, 0.0, Asset->Timeline.GetTotalLength()); }

	// SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(400.0, 110.0); }
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	// FEditorUndoClient
	virtual void PostUndo(bool bSuccess) override { ClampSelection(); }
	virtual void PostRedo(bool bSuccess) override { ClampSelection(); }

private:
	enum class EHit : uint8 { None, Ruler, Body, TrimLeft, TrimRight, Overlap, MarkerStart, MarkerEnd };
	struct FHit
	{
		EHit Type = EHit::None;
		int32 Index = INDEX_NONE;
	};
	enum class EDrag : uint8 { None, Scrub, Move, TrimLeft, TrimRight, Overlap, Pan, MarkerStart, MarkerEnd };

	int32 FindCutTarget() const;
	void BrowseToSelectedAsset();
	void OpenContextMenu(const FPointerEvent& Event);
	FHit HitTest(const FVector2f& LocalPos) const;
	float TimeToX(double Time) const { return float((Time - ViewStart) * PxPerSec); }
	double XToTime(float X) const { return ViewStart + X / PxPerSec; }
	void SelectClip(int32 Index, bool bOverlap);
	void ClampSelection();
	void OnDragMove(float LocalX);
	bool GetDroppedSequences(const FDragDropEvent& Event, TArray<UAnimSequence*>& Out, bool bLoad) const;
	int32 TimeToInsertIndex(double Time) const;

	static constexpr float RulerHeight = 24.f;
	static constexpr float TrackTop = 32.f;
	static constexpr float TrackHeight = 64.f;
	static constexpr float HandleWidth = 7.f;

	TObjectPtr<UAnimStitchTimelineAsset> Asset;
	FSimpleDelegate OnSelectionChangedDelegate;
	FSimpleDelegate OnPlayheadChangedDelegate;
	FAnimStitchWarningsDelegate OnWarningsDelegate;

	// View
	double ViewStart = -0.2;
	double PxPerSec = 100.0;
	double Playhead = 0.0;

	// Selection
	int32 SelectedClip = INDEX_NONE;
	bool bOverlapSelected = false;

	// Drag
	EDrag DragMode = EDrag::None;
	int32 DragIndex = INDEX_NONE;
	float DragStartX = 0.f;
	bool bDragStarted = false;
	double InitialTrimIn = 0.0;
	double InitialTrimOut = 0.0;
	double InitialOverlap = 0.0;
	double InitialStart = 0.0;
	double InitialViewStart = 0.0;
	TUniquePtr<FScopedTransaction> DragTransaction;

	// Drop feedback
	bool bDropHover = false;
	double DropTime = 0.0;
};


