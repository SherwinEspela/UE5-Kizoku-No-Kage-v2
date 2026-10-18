#pragma once

#include "CoreMinimal.h"
#include "Misc/FrameRate.h"
#include "Animation/AnimSequence.h"
#include "AnimStitchTimeline.generated.h"

/** Shape of the crossfade weight ramp across an overlap. */
UENUM()
enum class EAnimStitchBlendCurve : uint8
{
	Linear,
	Smoothstep,
};

/** One clip on the (single) track. Times are in seconds. Trimming is non-destructive. */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchClip
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimSequence> Sequence;

	/** Source in/out points (seconds into the source sequence). */
	UPROPERTY()
	double TrimIn = 0.0;

	UPROPERTY()
	double TrimOut = 0.0;

	/** How far this clip overlaps the previous one (the crossfade length). Always 0 for the first clip. */
	UPROPERTY()
	double OverlapIn = 0.0;

	/** Derived by FAnimStitchTimeline::Relayout(): position of the clip start on the timeline. */
	UPROPERTY()
	double StartTime = 0.0;

	/** Curve used for the crossfade INTO this clip from the previous one (ignored for the first clip). */
	UPROPERTY()
	EAnimStitchBlendCurve BlendIn = EAnimStitchBlendCurve::Smoothstep;

	double GetDuration() const { return TrimOut - TrimIn; }
	double GetEndTime() const { return StartTime + GetDuration(); }
};

/**
 * Plain description of a timeline. The same description feeds preview and bake.
 * Layout is derived: clips sit end-to-butt, each pulled back by its OverlapIn, so a timeline
 * can never contain gaps or out-of-order clips.
 */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchTimeline
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FAnimStitchClip> Clips;

	/** Index of the clip whose root motion drives the merge. */
	UPROPERTY()
	int32 RootAuthorityIndex = 0;

	/** Smallest allowed clip length after trimming. */
	static constexpr double MinClipDuration = 0.05;

	/** Clamps overlaps to legal values and recomputes every StartTime. */
	void Relayout();

	/** Largest legal OverlapIn for clip Index given the current neighbours. */
	double GetMaxOverlap(int32 Index) const;

	double GetTotalLength() const;
};

/**
 * Single source of truth for "what does the timeline look like at time T".
 * Used by the bake and (later phases) by the live preview.
 */
class ANIMSTITCHEDITOR_API FAnimStitchEvaluator
{
public:
	/** Validates the timeline and caches per-clip data. Returns false and fills OutError on failure. */
	bool Initialize(const FAnimStitchTimeline& InTimeline, FString& OutError);

	USkeleton* GetSkeleton() const { return Skeleton; }
	int32 GetNumBones() const { return NumBones; }

	/** Local-space (parent-relative) pose for every reference-skeleton bone at timeline time T. */
	void EvaluatePose(double TimelineTime, TArray<FTransform>& OutLocalPose) const;

	/** Cheap root-bone-only version of EvaluatePose (blended across overlaps). */
	FTransform EvaluateRoot(double TimelineTime) const;

	/** Pose of a single clip alone (no crossfade) with the authority-corrected root. */
	void EvaluateClipPose(int32 ClipIndex, double TimelineTime, TArray<FTransform>& OutLocalPose) const;

	int32 GetNumClips() const { return Timeline.Clips.Num(); }
	const FAnimStitchTimeline& GetTimeline() const { return Timeline; }

	/** Effective (root-motion-authority-corrected) root transform of clip ClipIndex at timeline time T. */
	FTransform EvaluateClipRoot(int32 ClipIndex, double TimelineTime) const;

private:
	void SampleClipRaw(int32 ClipIndex, double TimelineTime, TArray<FTransform>& OutPose) const;
	FTransform SampleClipRawRoot(int32 ClipIndex, double SourceTime) const;
	static float ApplyCurve(EAnimStitchBlendCurve Curve, float Alpha);

	FAnimStitchTimeline Timeline;
	USkeleton* Skeleton = nullptr;
	int32 NumBones = 0;
	/** Per clip, per skeleton bone: does the source have a track for it. */
	TArray<TArray<bool>> HasTrack;
	/** Root transform that clip i starts from (i > authority) / ends at (i < authority). */
	TArray<FTransform> RootAnchor;
};
