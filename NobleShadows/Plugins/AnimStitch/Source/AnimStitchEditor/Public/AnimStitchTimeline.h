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

/** What a manual foot-lock event does from its time FORWARD (until the next event for that foot). */
UENUM()
enum class EAnimStitchFootAction : uint8
{
	/** Glue the foot to its world position at this moment. Persists until an Unlock (or Auto) event. */
	Lock,
	/** Release the foot; stays released (no automatic plant detection) until the next Lock or Auto event. */
	Unlock,
	/** Hand the foot back to automatic plant detection from here on. */
	Auto,
};

/**
 * Manual foot-lock event. Stored on the clip in SOURCE time so it follows the clip when it is moved or trimmed.
 * Before a foot's first manual event, automatic detection applies; from the first event on, events decide.
 */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchFootEvent
{
	GENERATED_BODY()

	/** 0 = left foot, 1 = right foot. */
	UPROPERTY()
	int32 Foot = 0;

	/** Seconds into the clip's source sequence. */
	UPROPERTY()
	double SrcTime = 0.0;

	UPROPERTY()
	EAnimStitchFootAction Action = EAnimStitchFootAction::Lock;

	/** Lock events only: world-space offset the user dragged the pin by (free IK target). Zero = pinned where the foot was. */
	UPROPERTY()
	FVector PinOffset = FVector::ZeroVector;
};
/** Foot lock (post-blend plant pinning + two-bone IK) settings. OFF by default; flat ground plane assumed. */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchFootLockSettings
{
	GENERATED_BODY()

	UPROPERTY()
	bool bEnabled = false;

	/** Foot bones; None = use the remembered pair for the skeleton (or guess from bone names). */
	UPROPERTY()
	FName LeftFootBone;

	UPROPERTY()
	FName RightFootBone;

	/** Ground height = the feet's rest-pose height (the foot bone's height in the reference pose). */
	UPROPERTY()
	bool bAutoGroundHeight = true;

	/** Ground-plane height (cm, component/world Z of the foot bone when planted) when not automatic. */
	UPROPERTY()
	float GroundHeight = 0.f;

	/** A foot can be planted while its bone is at most this far (cm) above the ground height. */
	UPROPERTY()
	float HeightThreshold = 5.f;

	/** ...and moving no faster than this (cm/s) in world space (root motion included). */
	UPROPERTY()
	float VelocityThreshold = 40.f;

	/** Plants shorter than this (seconds) are ignored. */
	UPROPERTY()
	float MinPlantDuration = 0.1f;

	/** Correction fades in and out over this long (seconds) at the ends of a plant. */
	UPROPERTY()
	float BlendTime = 0.1f;

	/** Let the pelvis (and everything above it) follow so pinned legs are not stretched straight when the body moves away. */
	UPROPERTY()
	bool bPelvisFollows = true;

	/** Largest distance (cm) the pelvis may be moved from the animated position. */
	UPROPERTY()
	float MaxPelvisOffset = 20.f;
};

/** A prop shown on the preview character (preview only, never baked): a static or skeletal mesh on a socket or bone. */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchPreviewAttachment
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UObject> Asset;

	/** Socket of the preview mesh, or any bone name. */
	UPROPERTY()
	FName Socket;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector Scale = FVector::OneVector;

	/** Preview-only visibility toggle (kept in the timeline asset). */
	UPROPERTY()
	bool bVisible = true;
};
/** Manual bone correction track kinds. */
UENUM()
enum class EAnimStitchTrackKind : uint8
{
	/** Additive transform offsets on one bone (core bones, FK joints). */
	Bone,
	/** Free-IK limb: world-space effector (foot/hand) target, pole-vector target and IK weight. */
	IKLimb,
	/** The pole vector of an IK limb: its own lane under the IK track (Bone = the IK effector bone); keys hold the absolute pole position (Translation). */
	IKPole,
	/** The one root-motion offset lane of the timeline: additive translation + yaw on the root, applied WITH the blend and BEFORE foot lock. */
	RootOffset,
};

/** How a bone track's keys act. Fixed when the track is created. */
UENUM()
enum class EAnimStitchTrackMode : uint8
{
	/** Keys are additive offsets on the blended result (the original behaviour; also how older projects load). */
	Additive,
	/** Keys hold the bone's full transform relative to the root bone (so the pose travels with root motion) and replace the blended result across the keyed range. */
	Absolute,
};

/**
 * One keyframe of a manual correction track (timeline time).
 * Bone tracks: Translation / Rotation / Scale are ADDITIVE offsets (component space; scale is a factor, 1 = unchanged).
 * IK limb tracks: Translation / Rotation are the effector's world-space target, Pole is the world-space pole target,
 * Weight blends from the animation (0) to the target (1).
 */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchTrackKey
{
	GENERATED_BODY()

	UPROPERTY()
	double Time = 0.0;

	UPROPERTY()
	FVector Translation = FVector::ZeroVector;

	UPROPERTY()
	FQuat Rotation = FQuat::Identity;

	UPROPERTY()
	FVector Scale = FVector::OneVector;

	UPROPERTY()
	FVector Pole = FVector::ZeroVector;

	UPROPERTY()
	float Weight = 1.f;
};

/** Shape of a control in the viewport. */
UENUM()
enum class EAnimStitchControlShape : uint8
{
	Box,
	Sphere,
};

/** Per-control look. Overrides win over the global Show > Controls values; the offset only moves the drawn control (to separate overlapping shapes). */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchControlStyle
{
	GENERATED_BODY()

	UPROPERTY()
	bool bOverrideOpacity = false;

	UPROPERTY()
	float Opacity = 0.25f;

	UPROPERTY()
	bool bOverrideSize = false;

	/** Size multiplier used instead of the global Control Size while overridden. */
	UPROPERTY()
	float Size = 1.f;

	UPROPERTY()
	EAnimStitchControlShape Shape = EAnimStitchControlShape::Box;

	/** Display offset (cm, component space). */
	UPROPERTY()
	FVector Offset = FVector::ZeroVector;
};

/** A transform lane under the clip lanes (Sequencer style). Corrections are applied after foot prediction and foot lock: manual wins. */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchTrack
{
	GENERATED_BODY()

	UPROPERTY()
	EAnimStitchTrackKind Kind = EAnimStitchTrackKind::Bone;

	/** Bone tracks: the corrected bone. IK limb tracks: the end effector bone (foot / hand); thigh and calf are its two parents. */
	UPROPERTY()
	FName Bone;

	/** FK joints rotate only. */
	UPROPERTY()
	bool bRotationOnly = false;

	/** Bone tracks: when this bone moves the legs (pelvis etc.), keep the feet planted where the animation had them (leg IK). */
	UPROPERTY()
	bool bKeepFeetPlanted = true;

	/** Look of this track's control (bone control / IK effector) and, for IK limbs, of its pole control. */
	UPROPERTY()
	FAnimStitchControlStyle ControlStyle;

	UPROPERTY()
	FAnimStitchControlStyle PoleStyle;

	/** Bone tracks: Additive offsets (default; older projects) or Absolute full transforms. Chosen at creation, never changed afterwards. */
	UPROPERTY()
	EAnimStitchTrackMode Mode = EAnimStitchTrackMode::Additive;

	/** Tracks created together as the FK joints of one limb share a group (0 = none). */
	UPROPERTY()
	int32 GroupId = 0;

	/** Sorted by time. */
	UPROPERTY()
	TArray<FAnimStitchTrackKey> Keys;
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

	/** Stable identity (survives reordering), so correction keys can follow their clip. Assigned on demand. */
	UPROPERTY()
	FGuid ClipId;

	/** Derived by FAnimStitchTimeline::Relayout(): position of the clip start on the timeline. */
	UPROPERTY()
	double StartTime = 0.0;

	/** Curve used for the crossfade INTO this clip from the previous one (ignored for the first clip). */
	UPROPERTY()
	EAnimStitchBlendCurve BlendIn = EAnimStitchBlendCurve::Smoothstep;

	/** Batch mode: this slot is replaced by each of the user's source clips (everything else stays as in the template). */
	UPROPERTY()
	bool bBatchSlot = false;

	/** Manual foot lock / unlock events that live on this clip (source time). */
	UPROPERTY()
	TArray<FAnimStitchFootEvent> FootEvents;

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

	UPROPERTY()
	FAnimStitchFootLockSettings FootLock;

	/** Manual bone correction lanes (additive bone offsets, FK joints, free-IK limbs). Applied last. */
	UPROPERTY()
	TArray<FAnimStitchTrack> Tracks;

	/** Props attached to the preview character for reference (never baked). */
	UPROPERTY()
	TArray<FAnimStitchPreviewAttachment> PreviewAttachments;

	/** EXPERIMENTAL: make crossfades plant-aware (pre-blend foot prediction). Uses the foot bones/thresholds of FootLock. */
	UPROPERTY()
	bool bPredictFeet = false;

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

	/** Crossfade weight (curve applied, 0..1) of the incoming clip IncomingIndex at timeline time T; 0 outside its overlap. */
	float EvaluateBlendAlpha(int32 IncomingIndex, double TimelineTime) const;

	/** Pose of a single clip alone (no crossfade) with the authority-corrected root. */
	void EvaluateClipPose(int32 ClipIndex, double TimelineTime, TArray<FTransform>& OutLocalPose) const;

	int32 GetNumClips() const { return Timeline.Clips.Num(); }

	/** True if at least one clip animates this skeleton bone (otherwise it stays at the reference pose). */
	bool IsBoneAnimated(int32 BoneIndex) const
	{
		for (const TArray<bool>& Clip : HasTrack)
		{
			if (Clip.IsValidIndex(BoneIndex) && Clip[BoneIndex]) { return true; }
		}
		return false;
	}
	const FAnimStitchTimeline& GetTimeline() const { return Timeline; }

	/** Effective (root-motion-authority-corrected) root transform of clip ClipIndex at timeline time T. */
	FTransform EvaluateClipRoot(int32 ClipIndex, double TimelineTime) const;

private:
	void SampleClipRaw(int32 ClipIndex, double TimelineTime, TArray<FTransform>& OutPose) const;
	FTransform SampleClipRawRoot(int32 ClipIndex, double SourceTime) const;
	static float ApplyCurve(EAnimStitchBlendCurve Curve, float Alpha);
	/** Adds the Root offset track's value at a timeline time to a root transform (no-op without that track). */
	void ApplyRootOffset(double TimelineTime, FTransform& Root) const;

	FAnimStitchTimeline Timeline;
	int32 RootOffsetTrack = INDEX_NONE;
	USkeleton* Skeleton = nullptr;
	int32 NumBones = 0;
	/** Per clip, per skeleton bone: does the source have a track for it. */
	TArray<TArray<bool>> HasTrack;
	/** Root transform that clip i starts from (i > authority) / ends at (i < authority). */
	TArray<FTransform> RootAnchor;
};





