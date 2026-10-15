#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "AnimToolset.generated.h"

class UAnimNotify;
class UAnimSequence;
class UAnimSequenceBase;

/// Animation tools (read-only): sequence info, notifies, bones, bone trajectories, root motion summary.
UCLASS(BlueprintType)
class UAnimToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Returns a JSON string describing an animation sequence: length in seconds, frame rate, number of frames,
	 * rate scale, loop flag, root motion flags, additive type, skeleton and number of notifies.
	 * @param Sequence The animation sequence.
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString GetAnimInfo(UAnimSequenceBase* Sequence);

	/**
	 * Returns a JSON string with every notify on a sequence: name, trigger time (seconds), frame (at the sequence frame rate),
	 * duration (state notifies), track index, and the notify / notify state class path.
	 * @param Sequence The animation sequence.
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString ListNotifies(UAnimSequenceBase* Sequence);

	/**
	 * Returns a JSON string listing the skeleton's bones in order: index, name, parent index, and whether the sequence
	 * has an animated track for the bone.
	 * @param Sequence The animation sequence.
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString ListBones(UAnimSequence* Sequence);

	/**
	 * Returns the position of a bone in component space (cm, relative to the mesh origin; includes root motion if the
	 * root bone is animated) for a range of frames. JSON: {"bone","fps","startFrame","endFrame","positions":[[x,y,z],...]}.
	 * Frame i is at time i / fps. Bones without an animated track use the skeleton reference pose.
	 * @param Sequence The animation sequence.
	 * @param BoneName Name of the bone, e.g. foot_l.
	 * @param StartFrame First frame (0 based).
	 * @param EndFrame Last frame inclusive; -1 means the last frame.
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString GetBoneTrajectory(UAnimSequence* Sequence, const FString& BoneName, int32 StartFrame = 0, int32 EndFrame = -1);

	/**
	 * Returns a JSON summary of root motion measured from the first bone of the skeleton (the root): translation at the
	 * start and end, total travelled distance (cm), horizontal distance, average speed in cm/s over the sequence, and
	 * the average horizontal speed in cm/s. Uses the raw animation track (rate scale is NOT applied).
	 * @param Sequence The animation sequence.
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString GetRootMotionSummary(UAnimSequence* Sequence);

	/**
	 * Adds an instanced notify of the given class to a sequence at a time in seconds. Marks the asset dirty (does not save).
	 * Returns JSON with the new notify index. Does not add if a notify of the same class already exists within 1 ms.
	 * @param Sequence The animation sequence.
	 * @param NotifyClass Notify class, e.g. a Blueprint AnimNotify generated class (..._C).
	 * @param TimeSeconds Time of the notify in seconds, within the sequence length.
	 * @param TrackIndex Notify track to place it on (0 based).
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString AddNotify(UAnimSequenceBase* Sequence, TSubclassOf<UAnimNotify> NotifyClass, float TimeSeconds, int32 TrackIndex = 0);

	/**
	 * Removes the notify at the given index (see ListNotifies). Marks the asset dirty (does not save).
	 * @param Sequence The animation sequence.
	 * @param NotifyIndex Index in the Notifies array.
	 */
	UFUNCTION(meta = (AICallable), Category = "AnimToolset")
	static FString RemoveNotify(UAnimSequenceBase* Sequence, int32 NotifyIndex);
};
