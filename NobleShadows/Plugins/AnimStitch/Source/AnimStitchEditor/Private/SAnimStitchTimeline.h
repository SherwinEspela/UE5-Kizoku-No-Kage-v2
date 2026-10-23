#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "EditorUndoClient.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchFootLock.h"
#include "AnimStitchManualEdits.h"

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
	/** Shift+click builds a multi-selection of clips; the primary clip (GetSelectedClip) is one of them. */
	bool IsClipSelected(int32 Index) const { return Asset && Asset->Timeline.Clips.IsValidIndex(Index) && (Index == SelectedClip || SelectedClipIds.Contains(Asset->Timeline.Clips[Index].ClipId)); }
	int32 GetNumSelectedClips() const { return GetSelectedClipIndices().Num(); }
	TArray<int32> GetSelectedClipIndices() const;
	bool IsOverlapSelected() const { return bOverlapSelected; }

	void DeleteSelected();
	void DuplicateSelected();
	void SetSelectedBlendCurve(EAnimStitchBlendCurve Curve);
	void ZoomToFit();
	/** Fits the view to the range flags: the start flag at the left edge, the end flag at the right edge (a full-span range behaves like Zoom to Fit). */
	void ZoomToRange();
	/** Moves the range START (bStart) or END flag to the playhead (one undo step). */
	void SetRangeFlagToPlayhead(bool bStart);

	/** Latest foot-lock analysis shown in the plant lanes (null = none). */
	void SetPlants(TSharedPtr<const FAnimStitchFootLockResult> InPlants) { Plants = InPlants; }

	/** Appends sequences (skeleton-checked, undoable) and zooms to fit. Used by the Content Browser panel. */
	void AddSequencesToEnd(const TArray<UAnimSequence*>& Sequences) { AddSequencesAt(Sequences, INDEX_NONE); }

	/** Zoom to fit as soon as the widget has a real size (safe to call right after creating or loading). */
	void RequestZoomToFit() { bPendingZoomFit = true; }
	void ApplyPendingZoomFit()
	{
		// Resizing the panel (docking, floating, dragging a splitter) reframes the whole timeline to the new width.
		const float Width = GetTickSpaceGeometry().GetLocalSize().X;
		if (Width > 100.f && !FMath::IsNearlyEqual(Width, LastFitWidth, 1.f))
		{
			if (LastFitWidth > 0.f) { bPendingZoomFit = true; }
			LastFitWidth = Width;
		}
		if (bPendingZoomFit && GetTickSpaceGeometry().GetLocalSize().X > 100.f)
		{
			ZoomToFit();
			bPendingZoomFit = false;
		}
	}

	/** Points the widget at another timeline asset (New / Open). Resets selection and playhead. */
	void SetTimelineAsset(UAnimStitchTimelineAsset* NewAsset)
	{
		Asset = NewAsset;
		SelectedClip = INDEX_NONE;
		SelectedClipIds.Reset();
		bOverlapSelected = false;
		Playhead = 0.0;
		OnSelectionChangedDelegate.ExecuteIfBound();
	}

	// Cut commands act on the playhead: the selected clip if the playhead is inside it, else the topmost clip there.
	bool CanEditAtPlayhead() const { return FindCutTarget() != INDEX_NONE; }
	void SplitAtPlayhead();
	void TrimStartAtPlayhead();
	void TrimEndAtPlayhead();

	double GetPlayhead() const { return Playhead; }
	UAnimStitchTimelineAsset* GetAsset() const { return Asset; }

	// ---- Adding bone tracks (shared by the lane right-click menu and the main toolbar) ----
	void BuildCoreBoneMenu(class FMenuBuilder& Menu);
	/** Per-limb FK/IK switch at the playhead (one undo step). The limb is named by its hand / foot bone. */
	void SwitchLimbAtPlayhead(FName Effector, bool bToIK);
	/** Bake to Control Rig: reveals every control and converts every key to Absolute, as one undo step. A deliberate command, never run implicitly. */
	void BakeToControlRig();
	void BuildLimbMenu(class FMenuBuilder& Menu);
	/** Opens the bone picker window (bones already on the track list are not offered; hands and feet offer FK / IK). */
	void PickOtherBone();
	int32 GetSelectedTrack() const { return SelectedTrack; }
	/** Bumped every time a track is selected (clicked, added, jumped to), even when it is the same track again. */
	uint32 GetTrackSelectionSerial() const { return TrackSelectionSerial; }
	static bool HasKeyClipboard();
	/** Paste is only offered when the clipboard holds keys for a track that exists in this timeline. */
	bool CanPasteKeys() const;
	/** Copy + paste in one step; needs selected keys and a playhead that is not already on them. */
	bool CanDuplicateKeysToPlayhead() const;
	void DuplicateKeysToPlayhead() { CopySelectedKeys(); PasteKeys(); }
	/** Selects the clip that uses this sequence (the next one when the selected clip already does). False if it is not on the timeline. */
	bool SelectClipForSequence(const FSoftObjectPath& Path);
	/** Selecting a control in the viewport selects its track here (no key / control reselect loop). */
	void SelectTrackFromViewport(int32 Track);
	bool IsDraggingFootLock() const { return bDragStarted && (DragMode == EDrag::FootHandle || DragMode == EDrag::FootRegion); }

	// ---- Key value expander (numeric entry under a track lane) ----
	/** Width of the track column on the left (Sequencer style); the time area starts to its right. */
	float GetLabelWidth() const { return LabelWidth; }

	// ---- Vertical scrolling of the correction lanes ----
	float GetTrackAreaTop() const { return GetTracksTop(); }
	float GetTrackContentHeight() const { return Asset ? NumVisibleTracks() * (TrackLaneHeight + 2.f) + ExpandedCount() * ExpanderHeight + 8.f : 0.f; }

	// ---- Track search (Sequencer style) ----
	/** Live filter of the track list: only tracks whose displayed name (or lane names) contain the text are shown. Empty = every track. */
	void SetTrackFilter(const FString& Filter) { TrackFilter = Filter; VScroll = 0.f; }
	const FString& GetTrackFilter() const { return TrackFilter; }
	bool IsTrackVisible(int32 Track) const;
	bool MatchesTrackFilter(int32 Track) const;
	int32 NumVisibleTracks() const { int32 N = 0; if (Asset) { for (int32 i = 0; i < Asset->Timeline.Tracks.Num(); ++i) { N += IsTrackVisible(i) ? 1 : 0; } } return N; }
	int32 VisibleTracksBefore(int32 Track) const { int32 N = 0; if (Asset) { for (int32 i = 0; i < Track && i < Asset->Timeline.Tracks.Num(); ++i) { N += IsTrackVisible(i) ? 1 : 0; } } return N; }
	/** The title shown in the track column (also what the search matches). */
	static FString GetTrackLabel(const FAnimStitchTrack& Track);
	/** Height of the band above the first lane that holds the search field. */
	static constexpr float SearchBarHeight = 26.f;
	/** Pressing Esc in the timeline clears an active search (the panel empties its search box). */
	FSimpleDelegate OnTrackFilterCleared;
	float GetTrackViewHeight() const { return FMath::Max(0.f, GetCachedGeometry().GetLocalSize().Y - GetTracksTop()); }
	float GetTrackScrollMax() const { return FMath::Max(0.f, GetTrackContentHeight() - GetTrackViewHeight()); }
	float GetTrackScroll() const { return VScroll; }
	void SetTrackScroll(float Offset) { VScroll = FMath::Clamp(Offset, 0.f, GetTrackScrollMax()); }
	static constexpr float ExpanderHeight = 112.f;

	// ---- View range (used by the scroll and zoom bar at the bottom) ----
	void GetVisibleTimeRange(double& OutStart, double& OutEnd) const;
	/** Shows [Start, End] seconds in the time area. */
	void SetVisibleTimeRange(double Start, double End);
	/** Frames / seconds ruler units. */
	void SetShowFrames(bool bFrames) { bShowFrames = bFrames; }
	bool GetShowFrames() const { return bShowFrames; }
	double GetDisplayFps() const;
	/** Jumps to the next (+1) / previous (-1) key of the selected track, or of all tracks when none is selected. Returns false if there is none. */
	bool JumpToAdjacentKey(int32 Direction);
	// Every track expands on its own; expanding one never collapses another.
	bool IsTrackExpanded(int32 Track) const { return Asset && Asset->Timeline.Tracks.IsValidIndex(Track) && ExpandedTracks.Contains(TrackKey(Asset->Timeline.Tracks[Track])); }
	int32 ExpandedCount() const { int32 N = 0; if (Asset) { for (int32 i = 0; i < Asset->Timeline.Tracks.Num(); ++i) { N += (IsTrackExpanded(i) && IsTrackVisible(i)) ? 1 : 0; } } return N; }
	int32 ExpandedAbove(int32 Track) const { int32 N = 0; if (Asset) { for (int32 i = 0; i < Track && i < Asset->Timeline.Tracks.Num(); ++i) { N += (IsTrackExpanded(i) && IsTrackVisible(i)) ? 1 : 0; } } return N; }
	void ToggleTrackExpanded(int32 Track);
	void ExpandAllTracks();
	void CollapseAllTracks();
	/** Top (widget-local) of the value area under an expanded lane. */
	float GetTrackExpanderTop(int32 Track) const { return IsTrackExpanded(Track) ? GetTrackLaneTop(Track) + TrackLaneHeight + 2.f : 0.f; }
	/** The selected key of that track, or INDEX_NONE (the value panel then shows the value at the playhead). */
	int32 GetSelectedKeyFor(int32 Track) const { return SelectedTrack == Track ? SelectedKey : INDEX_NONE; }
	bool HasSelectedTrack() const { return Asset && Asset->Timeline.Tracks.IsValidIndex(SelectedTrack); }
	/** Enter / A: keys the evaluated value of every selected track at the playhead (undoable). Returns false when nothing is selected. */
	bool AddKeyAtPlayhead();
	/** F while the timeline has focus is not a timeline command: it frames the selection in the VIEWPORT (bound by the panel). */
	FSimpleDelegate OnFrameViewport;
	/** Live value of a track's control (IK targets, pole) for keys written when the limb is not keyed yet; set by the panel. */
	TFunction<bool(int32 TrackIndex, FAnimStitchTrackKey& OutKey)> LiveKeyProvider;
	static FString TrackKey(const FAnimStitchTrack& Track) { return FString::Printf(TEXT("%d:%s"), (int32)Track.Kind, *Track.Bone.ToString()); }
	/** Moves the playhead to the next / previous key of a lane and selects it. */
	void JumpToKey(int32 Track, int32 Direction);
	void SetPlayhead(double Time) { Playhead = FMath::Clamp(Time, 0.0, Asset->Timeline.GetTotalLength()); }

	// SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(400.0, 200.0); }
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override { HoverLocal = FVector2f(-1.f, -1.f); }
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
	enum class EHit : uint8 { None, Ruler, Body, TrimLeft, TrimRight, Overlap, MarkerStart, MarkerEnd, FootLane, TrackLane, TrackKey, TrackPrev, TrackNext, TrackExpand, TrackAddKey, Splitter, FootHandle, FootRegion };
	struct FHit
	{
		EHit Type = EHit::None;
		int32 Index = INDEX_NONE;
		/** Track keys: the key index. */
		int32 Sub = INDEX_NONE;
		/** Track keys / lanes: 0 = the combined summary lane, else the AnimStitchChannel bit of the channel lane that was hit. */
		uint8 Channel = 0;
	};
enum class EDrag : uint8 { None, Scrub, Move, TrimLeft, TrimRight, RateLeft, RateRight, Overlap, Pan, MarkerStart, MarkerEnd, FootLane, KeyMove, Marquee, Splitter, FootHandle, FootRegion };

	void AddSequencesAt(const TArray<UAnimSequence*>& Sequences, int32 Index);
	/** The custom-painted parts (clips, flags, lanes, lane buttons, keys) have no widgets of their own: one tooltip follows the hovered part. */
	FText GetHoverTipText() const;
	FVector2f HoverLocal = FVector2f(-1.f, -1.f);
	bool bPendingZoomFit = false;
	float LastFitWidth = 0.f;
	bool IsFootLaneVisible() const { return Asset->Timeline.FootLock.bEnabled; }
	float GetLaneTop(int32 Foot) const { return RulerHeight + 4.f + Foot * (LaneHeight + 2.f); }
	float GetTrackTop() const { return RulerHeight + 8.f + (IsFootLaneVisible() ? GetNumFootLanes() * (LaneHeight + 2.f) + 2.f : 0.f); }
	/** One lane per designated ground contact (2 until the first plant detection has run). */
	int32 GetNumFootLanes() const { return Plants.IsValid() && Plants->bValid ? Plants->Chains.Num() : 2; }
	/** Lane label: L / R for the first two contacts (the feet), else the contact bone name (or its number). */
	FString GetFootLaneLabel(int32 Foot, bool bLong) const
	{
		if (Foot == 0) { return bLong ? TEXT("Left foot lock") : TEXT("L"); }
		if (Foot == 1) { return bLong ? TEXT("Right foot lock") : TEXT("R"); }
		const FString Name = (Plants.IsValid() && Plants->FootBones.IsValidIndex(Foot)) ? Plants->FootBones[Foot].ToString() : FString::FromInt(Foot + 1);
		return bLong ? Name + TEXT(" lock") : Name;
	}
	void ToggleFootPlantAt(int32 Foot, double Time);

	// ---- Manual bone correction lanes (Phase 9) ----
	static constexpr float TrackLaneHeight = 30.f;
	float GetTracksTop() const { return GetTrackTop() + TrackHeight + 10.f + SearchBarHeight; }
	/** A track the search hides sits far off screen, so painting and hit testing (which compare against the lane top) never see it. */
	float GetTrackLaneTop(int32 Track) const { return IsTrackVisible(Track) ? GetTracksTop() + VisibleTracksBefore(Track) * (TrackLaneHeight + 2.f) + ExpandedAbove(Track) * ExpanderHeight - VScroll : -100000.f; }
	FString TrackFilter;
	/** Per-track visibility under the current search, rebuilt only when the filter or the tracks change (the lane layout asks for it many times a paint). */
	mutable TArray<uint8> VisibleCache;
	mutable FString VisibleCacheFilter;
	mutable uint32 VisibleCacheStamp = MAX_uint32;
	TSet<FString> ExpandedTracks;
	float LabelWidth = 290.f;
	uint32 TrackSelectionSerial = 0;
	float VScroll = 0.f;
	bool HasTrackFor(FName Bone, EAnimStitchTrackKind Kind) const;
	bool LimbTrackExists(bool bLeft, bool bLeg, bool bIK) const;
	void AddLimbForEffector(FName Effector, bool bIK, EAnimStitchTrackMode Mode = EAnimStitchTrackMode::Absolute);
	bool bShowFrames = true;
	void PaintLeftColumn(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G) const;
	mutable float CachedWidth = 600.f;
	bool GetLaneButton(float X, float Y, int32 Track, EHit& OutType) const;
	void PaintTrackLanes(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G) const;
	void OpenAddTrackMenu(const FPointerEvent& Event);
	void OpenTrackMenu(const FPointerEvent& Event, int32 Track, int32 Key, double Time, uint8 Channel = 0);
	void AddBoneTrack(FName Bone, bool bRotationOnly, int32 GroupId, EAnimStitchTrackMode Mode = EAnimStitchTrackMode::Absolute);
	/** The single Root offset track (additive translation + yaw on the root). Both entry points (menu entry and bone picker) land here. */
	void AddRootOffsetTrack();
	bool HasRootOffsetTrack() const;
	/** Name of the skeleton's root bone (bone 0), or None without clips. */
	FName GetRootBoneName() const;
	/** The key a new key at the playhead starts from: the interpolated value, or - absolute tracks, limbs without a key - the control as it is evaluated on screen. */
	FAnimStitchTrackKey MakeKeyAtPlayhead(int32 TrackIndex) const;
	bool HasSelectedKeys() const { return SelectedKeys.Num() > 0 || (Asset->Timeline.Tracks.IsValidIndex(SelectedTrack) && Asset->Timeline.Tracks[SelectedTrack].Keys.IsValidIndex(SelectedKey)); }
	void AddLimb(bool bLeft, bool bLeg, bool bIK);
	int32 SelectedTrack = INDEX_NONE;
	int32 SelectedKey = INDEX_NONE;

	// ---- Multi-key selection, marquee and clipboard (Phase 10) ----
	/** Channel: 0 = the key on the combined summary lane, else the AnimStitchChannel bit of the channel lane the key was picked on. */
	struct FKeyRef { int32 Track = INDEX_NONE; int32 Key = INDEX_NONE; uint8 Channel = 0; bool operator==(const FKeyRef& O) const { return Track == O.Track && Key == O.Key && Channel == O.Channel; } };
	/** Every selected key (the primary SelectedTrack / SelectedKey is one of them). */
	TArray<FKeyRef> SelectedKeys;
	TArray<FKeyRef> MarqueeInitial;
	double FootRegionStart = 0.0, FootRegionEnd = 0.0, FootRegionMin = 0.0, FootRegionMax = 0.0, FootAnchor = 0.0;
	bool bFootRegionHasEnd = false;
	double FootHandleTime = 0.0;
	double FootHandleMin = 0.0, FootHandleMax = 0.0;
	FVector2f MarqueeStart = FVector2f::ZeroVector;
	FVector2f MarqueeCurrent = FVector2f::ZeroVector;
	bool IsKeySelected(int32 Track, int32 Key, uint8 Channel = 0) const { return SelectedKeys.Contains(FKeyRef{ Track, Key, Channel }) || (Track == SelectedTrack && Key == SelectedKey && Channel == SelectedChannel); }
	/** Channel of the primary selected key (0 = combined lane). */
	uint8 SelectedChannel = 0;
	/** Bone tracks show a Translation / Rotation / Scale sub-lane per channel while expanded. */
	bool HasChannelLanes(int32 Track) const { return IsTrackExpanded(Track) && Asset->Timeline.Tracks[Track].Kind == EAnimStitchTrackKind::Bone; }
	float GetChannelRowHeight() const { return (ExpanderHeight - 2.f) / 4.f; }
	/** Top (widget-local) of channel sub-lane Row (0 = Translation, 1 = Rotation, 2 = Scale) under an expanded bone track. */
	float GetChannelRowTop(int32 Track, int32 Row) const { return GetTrackLaneTop(Track) + TrackLaneHeight + 2.f + Row * GetChannelRowHeight(); }
	static uint8 ChannelBitForRow(int32 Row) { return uint8(1 << Row); }
	/** Clicking empty timeline space: nothing stays selected (clip, track, keys), including the viewport control the track selected. */
	void DeselectAll();
	bool IsChannelEditable(int32 Track, uint8 Channel) const { return Asset->Timeline.Tracks.IsValidIndex(Track) && (FAnimStitchManualEdits::GetEditableChannels(Asset->Timeline.Tracks[Track]) & Channel) != 0; }
	void UpdateMarquee(const FVector2f& Local);
	void CopySelectedKeys();
	void PasteKeys();
	void DeleteSelectedKeys();
	double InitialKeyTime = 0.0;
	void OpenFootLaneMenu(const FPointerEvent& Event, int32 Foot, double Time);
	int32 FindCutTarget() const;
	void BrowseToSelectedAsset();
	void OpenContextMenu(const FPointerEvent& Event);
	FHit HitTest(const FVector2f& LocalPos) const;
	float TimeToX(double Time) const { return LabelWidth + float((Time - ViewStart) * PxPerSec); }
	double XToTime(float X) const { return ViewStart + (X - LabelWidth) / PxPerSec; }
	void SelectClip(int32 Index, bool bOverlap);
	void ClampSelection();
	void OnDragMove(float LocalX);
	bool GetDroppedSequences(const FDragDropEvent& Event, TArray<UAnimSequence*>& Out, bool bLoad) const;
	int32 TimeToInsertIndex(double Time) const;

	static constexpr float RulerHeight = 24.f;
	static constexpr float LaneHeight = 10.f;
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
	/** Ids of every selected clip (stable while clips are reordered). */
	TArray<FGuid> SelectedClipIds;
	bool bCollapseOnRelease = false;
	void ToggleClipInSelection(int32 Index);
	bool bOverlapSelected = false;

	// Drag
	EDrag DragMode = EDrag::None;
	int32 DragIndex = INDEX_NONE;
	float DragStartX = 0.f;
	bool bDragStarted = false;
double InitialTrimIn = 0.0;
	double InitialRate = 1.0;
	double InitialWidth = 0.0;
	double InitialTrimOut = 0.0;
	double InitialOverlap = 0.0;
	double InitialStart = 0.0;
	double InitialViewStart = 0.0;
	TUniquePtr<FScopedTransaction> DragTransaction;

	// Foot lane drag
	int32 DragFoot = 0;
	double FootDragStartTime = 0.0;
	double FootDragCurTime = 0.0;
	TSharedPtr<const FAnimStitchFootLockResult> Plants;

	// Drop feedback
	bool bDropHover = false;
	double DropTime = 0.0;
};







