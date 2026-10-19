#pragma once

#include "CoreMinimal.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchFootLock.h"

struct FReferenceSkeleton;

/**
 * Manual bone corrections (Phase 9). Tracks hold keys in timeline time; they layer over the blended result AFTER the automatic
 * passes (foot prediction, foot lock), so manual always wins. Preview and bake call the same Apply.
 */
class ANIMSTITCHEDITOR_API FAnimStitchManualEdits
{
public:
	/** Interpolated track values at a timeline time (constant before the first and after the last key). */
	static FAnimStitchTrackKey Sample(const FAnimStitchTrack& Track, double Time);

	/** Index of a key within Tolerance seconds of Time, or INDEX_NONE. */
	static int32 FindKey(const FAnimStitchTrack& Track, double Time, double Tolerance = 1.0 / 120.0);

	/** Inserts (or replaces the key at the same moment) keeping the keys sorted. Returns its index. */
	static int32 SetKey(FAnimStitchTrack& Track, const FAnimStitchTrackKey& Key);

	/**
	 * Applies all tracks to a skeleton-local pose (indexed by skeleton reference bone) at a timeline time.
	 * PlantFootBones (left, right; optional): when a bone track moves a leg (pelvis...), these feet stay where they were via leg IK,
	 * unless the foot has its own active IK track.
	 */
	static void Apply(const FReferenceSkeleton& RefSkeleton, const TArray<FAnimStitchTrack>& Tracks, double Time, TArray<FTransform>& LocalPose, const FName* PlantFootBones = nullptr);

	/** Where an IK pole sits by default for a pose: out from the knee/elbow, away from the leg axis (falls back to the reference pose, then forward). */
	static FVector DefaultPoleForPose(const FReferenceSkeleton& RefSkeleton, const FAnimStitchFootChain& Chain, const TArray<FTransform>& ComponentPose);

	/** Limb chain (thigh/calf/foot or upper arm/forearm/hand) ending at the effector bone. */
	static FAnimStitchFootChain ChainFor(const FReferenceSkeleton& RefSkeleton, FName EffectorBone);

	/** Where an IK pole control sits by default: out from the knee/elbow, away from the leg axis. */
	static FVector DefaultPole(const FReferenceSkeleton& RefSkeleton, const FAnimStitchFootChain& Chain, const TArray<FTransform>& LocalPose);

	/** Best guess for a hand bone from names; NAME_None if nothing plausible. */
	static FName GuessHandBone(const FReferenceSkeleton& RefSkeleton, bool bLeft);

	/** Pelvis / spine / neck / head style bones found by name, for the "Add Bone Track" menu. */
	static TArray<FName> GetCoreBoneSuggestions(const FReferenceSkeleton& RefSkeleton);

	static bool IsIdentityKey(const FAnimStitchTrackKey& Key);

	/** -1 = left, +1 = right, 0 = centre / unknown, guessed from the bone name. */
	static int32 GetBoneSide(FName Bone);
};
