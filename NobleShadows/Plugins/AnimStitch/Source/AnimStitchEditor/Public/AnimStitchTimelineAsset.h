#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchTimelineAsset.generated.h"

/**
 * Transactional container for a timeline. Every mutator calls Modify(), so edits made inside an
 * FScopedTransaction are undoable. (Phase 4 turns this into a saveable project asset.)
 */
UCLASS()
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
	void SetBlendCurve(int32 Index, EAnimStitchBlendCurve Curve);

	/** True if TimelineTime lies inside clip Index with at least the minimum clip length left on both sides. */
	bool CanCutClipAt(int32 Index, double TimelineTime) const;
	/** Splits the clip into two at TimelineTime. Returns false if the time is too close to a clip edge. */
	bool SplitClip(int32 Index, double TimelineTime);
	/** Removes the part of the clip before / after TimelineTime. */
	bool TrimClipStartTo(int32 Index, double TimelineTime);
	bool TrimClipEndTo(int32 Index, double TimelineTime);

	/** Bumped by every edit, undo and redo; lets the preview know when to re-evaluate. */
	uint32 GetEditCounter() const { return EditCounter; }

	virtual void PostTransacted(const FTransactionObjectEvent& TransactionEvent) override;

private:
	void Touch() { Modify(); ++EditCounter; }

	uint32 EditCounter = 0;
};
