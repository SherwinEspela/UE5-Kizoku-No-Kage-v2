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

	/** Project format version. 0 = saved before the 2.0 line (the field did not exist), 2 = 2.0: derived control hierarchy, curve control shapes, baked-rig reveal model. Older projects are migrated on load. */
	UPROPERTY()
	int32 ProjectFormatVersion = 0;
	static constexpr int32 CurrentFormatVersion = 2;

	UPROPERTY()
	FAnimStitchTimeline Timeline;

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	/** Brings a project saved by an older version up to the current format (idempotent). Called on load; public so tests can run it on a constructed project. */
	void MigrateToCurrentFormat();
	/** IK key edits: short weight ramps next to far-away IK-off keys, then every key keeps a full target (see FAnimStitchManualEdits::NormalizeIKKeys). */
	void ShapeIKRamps(FAnimStitchTrack& Track, double KeyTime);

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
	/** Removes several clips at once (indices into the current order). */
	void RemoveClips(const TArray<int32>& Indices);
	/** Moves the clips at Indices as one block (keeping their relative order) so it sits before the Target-th clip of the remaining ones. Overlaps around the move reset to 0. */
	void MoveClipBlock(const TArray<int32>& Indices, int32 TargetAmongRemaining);
	/** Gives every clip a unique stable id (selection and correction keys refer to clips by it). */
	void EnsureClipIds();
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
	/** Switches the Ground Feet pass on / off (one undoable change; the whole timeline is re-grounded). */
	void SetGroundFeet(bool bGround);
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
	/** Shows / hides the motion trail of a track's control(s). */
	void SetTrackShowTrail(int32 Track, bool bShow);
	/** Moves one manual foot-lock event (the start or end handle of a locked region) to another timeline time, keeping its action and pin offset. */
	void MoveFootEvent(int32 Foot, double FromTime, double ToTime);
	/** Moves the pin of a manual foot lock (free IK target). */
	void SetPinOffset(int32 ClipIndex, int32 EventIndex, const FVector& Offset);
	void AddPreviewAttachment(const FAnimStitchPreviewAttachment& Attachment);
	void RemovePreviewAttachment(int32 Index);
	void SetPreviewAttachmentVisible(int32 Index, bool bVisible);
	void SetPreviewAttachmentScale(int32 Index, const FVector& Scale);
	// ---- Internal baked control rig (2.0) ----
	// A complete Epic-style rig for the skeleton exists behind the scenes: the timeline's evaluated result baked per frame onto every rig node (one per
	// body bone). Nothing of it is displayed or editable until the user adds the control (a track); adding one reveals its node with the keys already in place.
	// "Bake to Control Rig" reveals all nodes at once. The bake is derived from the clips, so it is rebuilt when they change and is never saved.
	struct FRigBake
	{
		double Rate = 30.0;
		double TotalLength = 0.0;
		/** Skeleton-local blended pose per frame (frame f = f / Rate; the last sample is at TotalLength). */
		TArray<TArray<FTransform>> Poses;
	};
	/** The baked rig (rebuilt when stale); false when the timeline cannot be evaluated. */
	bool GetRigBake(FRigBake& Out) const;
	/** Rig nodes of the skeleton: the bones that get a control in the rig (body bones, root excluded: the root has its own offset lane). */
	TArray<FName> GetRigNodes() const;
	/** True once the node's control has been added (revealed). */
	bool IsRigNodeRevealed(FName Bone) const;
	/** A rig node's baked transform relative to the root bone at a frame of the bake; false for an unknown bone / frame. */
	bool GetRigNodeTransform(FName Bone, int32 Frame, FTransform& Out) const;
	/**
	 * Bake to Control Rig: reveals every node and converts every bone-track key to Absolute (per-frame keys that reproduce the current result).
	 * One undoable change when called inside a transaction. Returns the number of controls revealed or converted.
	 */
	struct FRigBakeOptions
	{
		/** true: a dense Absolute key on every frame (default). false: Absolute keys only at frames that already carry keys. */
		bool bEveryFrame = true;
		/** true: reveal every rig node and convert every control (default). false: only controls that already carry keys are converted; keyless controls stay hidden. */
		bool bAllControls = true;
	};
	int32 BakeToControlRig(const FRigBakeOptions& Options = FRigBakeOptions());

	/**
	 * Per-limb FK/IK switch, keyed at Time (call inside a transaction): the limb evaluates in its old mode before Time and in the new one from it until the next switch.
	 * Pose-matched, so nothing pops: FK->IK puts the IK target and pole at the limb's current evaluated pose (creating the IK track and pole lane if the limb had none);
	 * IK->FK initialises the FK joint controls (created if missing) from the current pose and carries that offset on while the limb stays in FK.
	 * Effector = the hand / foot bone. Returns false if the limb or timeline is invalid.
	 */
	bool SwitchLimbMode(FName Effector, bool bToIK, double Time);

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
	/** Absolute bone tracks never have a timeline span without per-frame keys: every frame with no key nearby gets a baseline key baked from the evaluated underlying animation (new or extended clips). Existing keys are never touched. */
	/** Spans the user emptied by deleting keys (Scope) are left alone: they interpolate between the surviving keys. */
	struct FKeyFollowScope;
	void FillAbsoluteTracks(const FKeyFollowScope* Scope = nullptr);
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
		/** A span of an Absolute track that was empty (keys deleted) before the edit: between two keys, before the first, or after the last. */
		struct FGuard { int32 Track = INDEX_NONE; int32 KeyA = INDEX_NONE; int32 KeyB = INDEX_NONE; FGuid EndClip; double EndSourceTime = 0.0; };
		UAnimStitchTimelineAsset& Asset;
		TArray<FOwner> Owners;
		TArray<FGuard> Guards;
		/** After Apply: per track, the timeline time ranges (open intervals) that must not be re-filled with the underlying animation. */
		TArray<TArray<TPair<double, double>>> Protected;
		bool bApplied = false;
	};
	friend struct FKeyFollowScope;

	bool bUnsavedChanges = false;
	mutable FRigBake CachedRig;
	mutable uint32 CachedRigStamp = MAX_uint32;
	uint32 EditCounter = 0;
	uint32 ManualCounter = 0;
};





