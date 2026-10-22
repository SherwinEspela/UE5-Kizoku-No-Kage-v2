#pragma once

#include "CoreMinimal.h"
#include "AnimStitchTimeline.h"

struct FReferenceSkeleton;

/** Leg bones of one foot: Thigh -> Calf -> Foot. */
struct ANIMSTITCHEDITOR_API FAnimStitchFootChain
{
	int32 Foot = INDEX_NONE;
	int32 Calf = INDEX_NONE;
	int32 Thigh = INDEX_NONE;
	int32 ThighParent = INDEX_NONE;

	bool IsValid() const { return Foot != INDEX_NONE && Calf != INDEX_NONE && Thigh != INDEX_NONE; }
};

/** Per-frame plant state. */
namespace AnimStitchPlant
{
	constexpr uint8 None = 0;
	/** Detected by the velocity + height heuristic. */
	constexpr uint8 Auto = 1;
	/** Forced planted by a manual override. */
	constexpr uint8 ManualPin = 2;
	/** Detected, but suppressed by a manual override. */
	constexpr uint8 Suppressed = 3;

	inline bool IsPlanted(uint8 State) { return State == Auto || State == ManualPin; }
}

struct ANIMSTITCHEDITOR_API FAnimStitchFootLockResult
{
	bool bValid = false;
	/** Why foot lock can't run (bone names missing, no leg chain...). */
	FText Problem;
	double Fps = 30.0;
	double GroundZ = 0.0;
	FName FootBones[2];
	FAnimStitchFootChain Chains[2];
	/** One AnimStitchPlant value per frame, per foot (0 = left, 1 = right). */
	TArray<uint8> Mask[2];

	/** Manual events mapped to timeline time (for drawing). */
	struct FEventMark
	{
		double Time = 0.0;
		EAnimStitchFootAction Action = EAnimStitchFootAction::Lock;
	};
	TArray<FEventMark> Events[2];

	/** Per frame: pin offset of the manual lock governing that frame (zero otherwise) and which event owns it (clip, event index; -1 = none). */
	TArray<FVector> PinOffset[2];
	TArray<FIntPoint> PinEvent[2];
};

/** Ground Feet pass tuning. */
namespace AnimStitchGroundFeet
{
	/** A plant whose average tilt (pitch/roll from level) is within this many degrees is an accidental tilt and is levelled; beyond it (toe / heel plants) the rotation is kept. */
	constexpr double AccidentalTiltDegrees = 25.0;
}

/** Extra results of FAnimStitchFootLock::Pin. */
struct ANIMSTITCHEDITOR_API FAnimStitchPinOutput
{
	/** Per frame: the world-space point a manually locked foot is pinned to (valid where the plant mask says ManualPin). */
	TArray<FVector> ManualTarget[2];
};

/** What the pre-blend foot prediction did (for logs and tests). */
struct ANIMSTITCHEDITOR_API FAnimStitchPredictReport
{
	bool bValid = false;
	FText Problem;
	int32 Overlaps = 0;
	/** Foot-frames that were moved onto a prediction target. */
	int32 PinnedFootFrames = 0;
	/** Foot-frames where a target was dropped/faded because the pelvis was out of the leg's reach. */
	int32 ReachReleasedFrames = 0;
	/** Worst horizontal distance (cm) of a fully-weighted foot to its target, before and after the solve. */
	double MaxDeviationBefore = 0.0;
	double MaxDeviationAfter = 0.0;
};

/**
 * Foot lock pass: detects planted feet in the blended per-frame transforms, pins each planted foot's world
 * position (root motion included) and re-solves the leg with analytic two-bone IK. Flat ground assumed.
 * Operates on local-space poses indexed by skeleton reference bone, exactly what FAnimStitchEvaluator produces.
 */
class ANIMSTITCHEDITOR_API FAnimStitchFootLock
{
public:
	/** Best guess for a skeleton's foot bone from bone names; NAME_None if nothing plausible. */
	static FName GuessFootBone(const FReferenceSkeleton& RefSkeleton, bool bLeft);

	/** Settings value if set, else the pair remembered for the skeleton, else a name-based guess. */
	static void ResolveFootBones(const USkeleton* Skeleton, const FAnimStitchFootLockSettings& Settings, FName& OutLeft, FName& OutRight);

	static FAnimStitchFootChain BuildChain(const FReferenceSkeleton& RefSkeleton, FName FootBone);

	/** Finds plants in the given per-frame poses (frame f = timeline time f / Fps), honouring manual overrides. */
	static FAnimStitchFootLockResult Detect(const class FAnimStitchEvaluator& Evaluator, const TArray<TArray<FTransform>>& Poses, double Fps);

	/**
	 * EXPERIMENTAL pre-blend foot prediction. Detects plants on each source clip on its own (unmixed motion), then, inside every
	 * crossfade, holds the outgoing clip's plants and settles the feet onto the incoming clip's stance, by re-solving the legs of the
	 * already-blended Poses. The pelvis is only read (to release a pin gracefully when it moves out of leg reach), never moved.
	 */
	static FAnimStitchPredictReport PredictFeet(const class FAnimStitchEvaluator& Evaluator, TArray<TArray<FTransform>>& Poses, double Fps);

	/** Pins planted feet by solving the legs; modifies Poses in place. Returns the number of corrected foot-frames. */
	static int32 Pin(const class FAnimStitchEvaluator& Evaluator, TArray<TArray<FTransform>>& Poses, const FAnimStitchFootLockResult& Detection, FAnimStitchPinOutput* Output = nullptr);

	/**
	 * Ground Feet pass: for every planted foot (Detection must come from Detect on the same poses) seats the sole on the ground plane and, when the
	 * plant counts as accidentally tilted (AnimStitchGroundFeet::AccidentalTiltDegrees), levels pitch/roll while keeping the heading. Plants tilted
	 * beyond that keep their rotation and are only seated by height. Swing feet are untouched. Edges ease in like foot lock. Returns the number of corrected foot-frames.
	 */
	static int32 GroundFeet(const class FAnimStitchEvaluator& Evaluator, TArray<TArray<FTransform>>& Poses, const FAnimStitchFootLockResult& Detection, double TiltToleranceDegrees = AnimStitchGroundFeet::AccidentalTiltDegrees);

	/**
	 * Two-bone IK for one limb (thigh/calf/foot or upper arm/forearm/hand): moves the effector to DesiredEffector, keeping segment lengths
	 * (the limb extends fully if out of reach, the pelvis/shoulder is never moved). PoleTarget (optional) steers the knee/elbow, EffectorRotation
	 * (optional, component space) sets the end bone orientation. Only rotations of the three bones change.
	 */
	static void SolveLimb(const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& LocalPose, const FAnimStitchFootChain& Chain, const FVector& DesiredEffector,
		const FVector* PoleTarget = nullptr, double PoleWeight = 0.0, const FQuat* EffectorRotation = nullptr);

	/** Component-space transform of a bone from a skeleton-local pose. */
	static FTransform GetComponentSpace(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& LocalPose, int32 Bone);
};




