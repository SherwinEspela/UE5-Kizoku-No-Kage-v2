#pragma once

#include "CoreMinimal.h"
#include "AnimStitchTimeline.h"

/** How a batch run names and writes its outputs. */
struct ANIMSTITCHEDITOR_API FAnimStitchBatchOptions
{
	/** Content folder for the baked sequences, e.g. /Game/Animations/Baked. */
	FString OutputFolder;
	/** Output name = Prefix + the source clip names joined with '_' + Suffix. */
	FString Prefix = TEXT("AS_");
	FString Suffix;
	/** Output frame rate in fps; 0 = the first clip's source rate. */
	int32 FrameRate = 0;
	/** Variable slots use each replacement clip's full length instead of the template's trims. */
	bool bFullLengthForBatchSlots = false;
	bool bSavePackages = true;
	/** Replace outputs that already exist (same skeleton) instead of reporting a failure. */
	bool bOverwriteExisting = false;
};

struct ANIMSTITCHEDITOR_API FAnimStitchBatchResult
{
	TArray<FString> CreatedAssets;
	TArray<FText> Failures;
};

/**
 * Applies a template timeline (clip slots, trims, crossfades, root authority) across sets of source clips.
 * Slots flagged bBatchSlot are replaced; every other slot stays as in the template. Row r uses the r-th clip
 * of every batch slot ("zipped"), so all slot lists must have the same length.
 */
class ANIMSTITCHEDITOR_API FAnimStitchBatch
{
public:
	static void GetBatchSlots(const FAnimStitchTimeline& Template, TArray<int32>& OutSlotIndices);

	/** Checks the assignments are usable. OutError is user-facing. */
	static bool Validate(const FAnimStitchTimeline& Template, const TArray<TArray<UAnimSequence*>>& Assignments, FText& OutError);

	/** Timeline for one row; RowClips[k] replaces the k-th batch slot. */
	static FAnimStitchTimeline BuildRowTimeline(const FAnimStitchTimeline& Template, const TArray<UAnimSequence*>& RowClips, bool bFullLength);

	static FString MakeOutputName(const FAnimStitchBatchOptions& Options, const TArray<UAnimSequence*>& RowClips);

	/** Bakes one sequence per row. Shows a progress bar and honours cancel. */
	static FAnimStitchBatchResult Run(const FAnimStitchTimeline& Template, const TArray<TArray<UAnimSequence*>>& Assignments, const FAnimStitchBatchOptions& Options);
};

