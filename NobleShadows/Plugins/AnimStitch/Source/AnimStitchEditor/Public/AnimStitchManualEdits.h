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

	/** Does the (bone) track change the pose at this time? Additive: any non-identity offset; absolute: inside the keyed range. */
	static bool IsActiveAt(const FAnimStitchTrack& Track, double Time);

	/** Index of a key within Tolerance seconds of Time, or INDEX_NONE. */
	static int32 FindKey(const FAnimStitchTrack& Track, double Time, double Tolerance = 1.0 / 120.0);

	/** Inserts (or replaces the key at the same moment) keeping the keys sorted. Returns its index. */
	static int32 SetKey(FAnimStitchTrack& Track, const FAnimStitchTrackKey& Key);

	// ---- Per-channel keys (bone tracks): every key holds a subset of Translation / Rotation / Scale (AnimStitchChannel bits) ----
	/** Channels that exist on this track (FK joints rotate only). */
	static uint8 GetApplicableChannels(const FAnimStitchTrack& Track);
	/** Channels whose lane is unlocked (bone tracks; every other track kind is always editable). */
	static uint8 GetEditableChannels(const FAnimStitchTrack& Track);
	/** Channels that change the pose at a time: additive = every channel that has keys; absolute = channels inside their own keyed range. */
	static uint8 GetActiveChannels(const FAnimStitchTrack& Track, double Time);
	/** Writes only the masked channels of Key into the key at its time (creating a key that holds just those channels if there is none). Returns its index. */
	static int32 SetChannelKey(FAnimStitchTrack& Track, const FAnimStitchTrackKey& Key, uint8 Channels);

	/**
	 * Applies all tracks to a skeleton-local pose (indexed by skeleton reference bone) at a timeline time.
	 * PlantFootBones (left, right; optional): when a bone track moves a leg (pelvis...), these feet stay where they were via leg IK,
	 * unless the foot has its own active IK track.
	 */
	static void Apply(const FReferenceSkeleton& RefSkeleton, const TArray<FAnimStitchTrack>& Tracks, double Time, TArray<FTransform>& LocalPose, const TArray<FName>* PlantFootBones = nullptr);

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

	/** IK tracks: gives every key that has no target (anchors / half-faded keys saved by older builds) the target of its nearest neighbour that has one. Idempotent; returns true if anything changed. Runs on load and after every IK key edit. */
	static bool NormalizeIKKeys(FAnimStitchTrack& Track);

	// ---- Per-limb FK/IK switch (hotfix set 6) ----
	/** The hand / foot bone of the limb a track belongs to: an IK track names it directly, an FK joint track is the effector or sits up to two levels above it. NAME_None if there is none. */
	static FName FindLimbEffector(const FReferenceSkeleton& RefSkeleton, FName Bone);
	/** True when the limb of this IK track evaluates in IK mode at a time (always true for a limb that was never switched). */
	static bool IsLimbIKAt(const FAnimStitchTrack& IKTrack, double Time);
	/** Index of the IK track of a limb (by effector bone), or INDEX_NONE. */
	static int32 FindIKTrack(const TArray<FAnimStitchTrack>& Tracks, FName Effector);

	/**
	 * The control hierarchy, derived live from the skeleton: the parent of Tracks[TrackIndex] is the Bone track on the nearest ancestor bone that has one
	 * (INDEX_NONE = top level). Because it is derived, it holds for every control whenever it was added, in any order.
	 */
	static int32 ResolveParentTrack(const FReferenceSkeleton& RefSkeleton, const TArray<FAnimStitchTrack>& Tracks, int32 TrackIndex);

	/** True for bones that get a node in the internal baked rig (body bones; twist / corrective / helper / IK helper / end / face bones have none). */
	static bool IsRigBone(FName Bone);

	/**
	 * True when a skeleton bone may have a control: a real BONE that passes IsRigBone. Sockets (skeleton or preview-mesh sockets, socket-style names) and virtual bones
	 * are never controls - not in the internal rig, not in the add-control picker, not in Bake to Control Rig.
	 */
	static bool IsControlBone(const class USkeleton* Skeleton, FName Bone);

	/**
	 * Bake to Control Rig, key conversion: re-expresses every Bone track as an Absolute track with one key per frame, so Apply on the result reproduces the pose
	 * Apply produced with the original tracks. BlendPoses are the per-frame skeleton-local poses BEFORE manual corrections (indexed by frame at Rate).
	 * IK, pole and root-offset tracks are left as they are.
	 * OnlyTracks (optional): convert just these track indices. bEveryFrame false: each converted track keeps Absolute keys only at the frames (and the first / last frame) where it already had a key.
	 */
	static void ConvertTracksToAbsolute(const FReferenceSkeleton& RefSkeleton, TArray<FAnimStitchTrack>& Tracks, const TArray<TArray<FTransform>>& BlendPoses, double Rate, double TotalLength, const TArray<FName>* PlantFootBones = nullptr, const TArray<int32>* OnlyTracks = nullptr, bool bEveryFrame = true);

	/** -1 = left, +1 = right, 0 = centre / unknown, guessed from the bone name. */
	static int32 GetBoneSide(FName Bone);

	/** True for the pelvis / hips bone (the only bone track that offers Keep Feet Planted). */
	static bool IsPelvisBone(FName Bone);
};
