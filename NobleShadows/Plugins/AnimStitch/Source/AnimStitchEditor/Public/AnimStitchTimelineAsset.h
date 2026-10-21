#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchTimelineAsset.generated.h"

/**
 * Transactional container for a timeline. Every mutator calls Modify(), so edits made inside an
 * FScopedTransaction are undoable. (Phase 4 turns this into a saveable project asset.)
 */
UCLASS(BlueprintType)
class ANIMSTITCHEDITOR_API UAnimStitchTimelineAsset : public UObject
{
	GENERATED_BODY()

public:
	UAnimStitchTimelineAsset();

	UPROPERTY()
	FAnimStitchTimeline Timeline;

	/** Playback range start (seconds). Only affects preview playback, never the bake. */
	UPROPERTY()
	double PlaybackStart = 0.0;

	/** Playback range end (seconds); negative means "end of the timeline". */
	UPROPERTY()
	double PlaybackEnd = -1.0;

	/** Effective playback range, clamped to the current timeline length. */
	void GetPlaybackRange(double& OutStart, double& OutEnd) const;
	void SetPlaybackRange(double NewStart, double NewEnd);

	/** Skeleton shared by all clips, or null for an empty timeline. */
	USkeleton* GetSkeleton() const;

	/**
	 * Appends/inserts the sequences at InsertIndex (INDEX_NONE = end). Sequences whose skeleton differs from
	 * the timeline's are skipped and described in OutWarnings. Returns the number added.
	 */
	int32 AddSequences(const TArray<UAnimSequence*>& Sequences, int32 InsertIndex, TArray<FText>& OutWarnings);

	void RemoveClip(int32 Index);
	/** Inserts a copy directly after the source clip (butt-joined). Returns the new index. */
	int32 DuplicateClip(int32 Index);
	/** Moves a clip to a new position in the order. Affected overlaps reset to 0. */
	void MoveClip(int32 From, int32 To);

	void SetOverlap(int32 Index, double NewOverlap);
void SetTrim(int32 Index, double NewTrimIn, double NewTrimOut);
	/** Per-clip play rate (clamped 0.1..4). The trimmed content is unchanged; the clip's width on the timeline becomes content / rate. */
	void SetClipRate(int32 Index, double NewRate);
	void SetBlendCurve(int32 Index, EAnimStitchBlendCurve Curve);
	void SetBatchSlot(int32 Index, bool bBatchSlot);

	void SetFootLockSettings(const FAnimStitchFootLockSettings& Settings);
	void SetPredictFeet(bool bPredict);
	/** Adds a manual foot event at a timeline time (replacing an event at the same moment). It applies from there forward. */
	void AddFootEvent(int32 Foot, double TimelineTime, EAnimStitchFootAction Action);
	/** Drag-paint: Lock at TimelineStart and an explicit Unlock at TimelineEnd; events in between are replaced. */
	void PaintFootLock(int32 Foot, double TimelineStart, double TimelineEnd);
	/** Removes all manual events for one foot (0/1) or both (INDEX_NONE): back to automatic detection. */
	void ClearFootEvents(int32 Foot);
	// ---- Manual bone correction tracks (Phase 9) ----
	/** Adds a track (a second track for the same bone and kind is refused). Returns its index. */
	int32 AddTrack(const FAnimStitchTrack& Track);
	void RemoveTrack(int32 Index);
	/** Sets (or replaces) the key at the key's time. The first key of a track also adds the anchor that keeps the correction local. */
	void SetTrackKey(int32 Track, const FAnimStitchTrackKey& Key);
	void RemoveTrackKey(int32 Track, int32 KeyIndex);
	/** Returns the key's index after re-sorting. */
	int32 MoveTrackKey(int32 Track, int32 KeyIndex, double NewTime);
	void SetTrackKeyWeight(int32 Track, int32 KeyIndex, float Weight);
	// ---- Per-channel keys (bone tracks). Other track kinds fall back to the whole-key calls above and ignore the mask. ----
	// Locked channels (see SetTrackChannelUnlocked) are skipped: they are never written, moved or deleted by these calls.
	/** Writes only the masked channels of Key at the key's time. */
	void SetTrackChannelKey(int32 Track, const FAnimStitchTrackKey& Key, uint8 Channels);
	/** Removes the masked channels from a key; the key disappears when no channel is left. */
	void RemoveTrackChannelKey(int32 Track, int32 KeyIndex, uint8 Channels);
	/** Moves the masked channels of a key to a new time (the other channels stay). Returns the index of the key now holding them. */
	int32 MoveTrackChannelKey(int32 Track, int32 KeyIndex, uint8 Channels, double NewTime);
	/** Locks / unlocks one channel lane of a bone track for editing. */
	void SetTrackChannelUnlocked(int32 Track, uint8 Channel, bool bUnlocked);
	/** Bone tracks: keep the feet planted (leg IK) while this bone moves the legs. */
	void SetTrackKeepFeet(int32 Track, bool bKeep);
	/** Sets the per-control look of a track (bPole = the IK pole control, otherwise the bone control / effector). */
	void SetTrackControlStyle(int32 Track, bool bPole, const FAnimStitchControlStyle& Style);
	/** Moves one manual foot-lock event (the start or end handle of a locked region) to another timeline time, keeping its action and pin offset. */
	void MoveFootEvent(int32 Foot, double FromTime, double ToTime);
	/** Moves the pin of a manual foot lock (free IK target). */
	void SetPinOffset(int32 ClipIndex, int32 EventIndex, const FVector& Offset);
	void AddPreviewAttachment(const FAnimStitchPreviewAttachment& Attachment);
	void RemovePreviewAttachment(int32 Index);
	void SetPreviewAttachmentVisible(int32 Index, bool bVisible);
	void SetPreviewAttachmentScale(int32 Index, const FVector& Scale);
	/** Bumped by every track/key edit (and undo/redo): the preview re-applies the manual corrections. */
	uint32 GetManualCounter() const { return ManualCounter; }

	/** Gives clip Index a crossfade of the plugin-settings default length (clamped to what fits). */
	void AddDefaultCrossfade(int32 Index);
	void SetRootAuthority(int32 Index);

	/** True if TimelineTime lies inside clip Index with at least the minimum clip length left on both sides. */
	bool CanCutClipAt(int32 Index, double TimelineTime) const;
	/** Splits the clip into two at TimelineTime. Returns false if the time is too close to a clip edge. */
	bool SplitClip(int32 Index, double TimelineTime);
	/** Removes the part of the clip before / after TimelineTime. */
	bool TrimClipStartTo(int32 Index, double TimelineTime);
	bool TrimClipEndTo(int32 Index, double TimelineTime);

	/** Bumped by every edit, undo and redo; lets the preview know when to re-evaluate. */
	uint32 GetEditCounter() const { return EditCounter; }

	/** True after any edit (or undo / redo) since the last save or load; drives the Save button's asterisk. */
	bool HasUnsavedChanges() const { return bUnsavedChanges; }
	void ClearUnsavedChanges() { bUnsavedChanges = false; }

	virtual void PostTransacted(const FTransactionObjectEvent& TransactionEvent) override;

private:
	/** Component-space transform of a bone as the BLEND alone produces it (no manual tracks) at a timeline time; fills Translation / Rotation / Scale of the key. */
	bool CaptureBlendedBoneTransform(FName Bone, double Time, FAnimStitchTrackKey& InOutKey) const;
	void Touch() { Modify(); ++EditCounter; bUnsavedChanges = true; }
	/** Manual track edits don't change what the automatic passes see, so they only bump this counter. */
	void TouchManual() { Modify(); ++ManualCounter; bUnsavedChanges = true; }
	/** Track edits: the Root offset track feeds the blend itself (and so foot lock), so it needs the full re-evaluation (Touch); others only the manual pass. */
	void TouchTrack(int32 TrackIndex)
	{
		if (Timeline.Tracks.IsValidIndex(TrackIndex) && Timeline.Tracks[TrackIndex].Kind == EAnimStitchTrackKind::RootOffset) { Touch(); ++ManualCounter; }
		else { TouchManual(); }
	}

	/** While alive, remembers which clip owns every correction key and, when applied (or destroyed), re-places the keys so they follow their clip. */
	struct FKeyFollowScope
	{
		explicit FKeyFollowScope(UAnimStitchTimelineAsset& InAsset);
		~FKeyFollowScope() { Apply(); }
		void Apply();
		struct FOwner { int32 Track = INDEX_NONE; int32 Key = INDEX_NONE; FGuid Clip; double SourceTime = 0.0; };
		UAnimStitchTimelineAsset& Asset;
		TArray<FOwner> Owners;
		bool bApplied = false;
	};
	friend struct FKeyFollowScope;

	bool bUnsavedChanges = false;
	uint32 EditCounter = 0;
	uint32 ManualCounter = 0;
};





