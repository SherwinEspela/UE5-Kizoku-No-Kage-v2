#include "AnimStitchBatch.h"
#include "AnimStitchBaker.h"
#include "Misc/ScopedSlowTask.h"
#include "Animation/AnimData/IAnimationDataModel.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void FAnimStitchBatch::GetBatchSlots(const FAnimStitchTimeline& Template, TArray<int32>& OutSlotIndices)
{
	OutSlotIndices.Reset();
	for (int32 i = 0; i < Template.Clips.Num(); ++i)
	{
		if (Template.Clips[i].bBatchSlot)
		{
			OutSlotIndices.Add(i);
		}
	}
}

bool FAnimStitchBatch::Validate(const FAnimStitchTimeline& Template, const TArray<TArray<UAnimSequence*>>& Assignments, FText& OutError)
{
	TArray<int32> Slots;
	GetBatchSlots(Template, Slots);
	if (Slots.Num() == 0)
	{
		OutError = LOCTEXT("BatchNoSlots", "No clip is marked as a Batch Slot. Right-click a clip on the timeline and enable Batch Slot.");
		return false;
	}
	if (Assignments.Num() != Slots.Num())
	{
		OutError = LOCTEXT("BatchSlotCount", "Every batch slot needs a list of source clips.");
		return false;
	}
	const int32 Rows = Assignments[0].Num();
	if (Rows == 0)
	{
		OutError = LOCTEXT("BatchNoClips", "Add at least one source clip to each batch slot.");
		return false;
	}
	for (const TArray<UAnimSequence*>& List : Assignments)
	{
		if (List.Num() != Rows)
		{
			OutError = LOCTEXT("BatchUneven", "All batch slots need the same number of source clips (clip N of each slot is baked together).");
			return false;
		}
	}
	return true;
}

FAnimStitchTimeline FAnimStitchBatch::BuildRowTimeline(const FAnimStitchTimeline& Template, const TArray<UAnimSequence*>& RowClips, bool bFullLength)
{
	FAnimStitchTimeline Row = Template;
	TArray<int32> Slots;
	GetBatchSlots(Template, Slots);
	for (int32 k = 0; k < Slots.Num() && k < RowClips.Num(); ++k)
	{
		FAnimStitchClip& Clip = Row.Clips[Slots[k]];
		UAnimSequence* Source = RowClips[k];
		Clip.Sequence = Source;
		Clip.FootEvents.Reset(); // manual foot events belong to the template's original clip, not its replacement
		const double Length = Source ? Source->GetPlayLength() : 0.0;
		if (bFullLength)
		{
			Clip.TrimIn = 0.0;
			Clip.TrimOut = Length;
		}
		else
		{
			// Keep the template's trims, clamped into the new clip.
			Clip.TrimIn = FMath::Clamp(Clip.TrimIn, 0.0, FMath::Max(0.0, Length - FAnimStitchTimeline::MinClipDuration));
			Clip.TrimOut = FMath::Clamp(Clip.TrimOut, Clip.TrimIn + FAnimStitchTimeline::MinClipDuration, Length);
		}
	}
	Row.Relayout();
	return Row;
}

FString FAnimStitchBatch::MakeOutputName(const FAnimStitchBatchOptions& Options, const TArray<UAnimSequence*>& RowClips)
{
	FString Joined;
	for (const UAnimSequence* Clip : RowClips)
	{
		Joined += (Joined.IsEmpty() ? TEXT("") : TEXT("_")) + GetNameSafe(Clip);
	}
	return Options.Prefix + Joined + Options.Suffix;
}

FAnimStitchBatchResult FAnimStitchBatch::Run(const FAnimStitchTimeline& Template, const TArray<TArray<UAnimSequence*>>& Assignments, const FAnimStitchBatchOptions& Options)
{
	FAnimStitchBatchResult Result;
	FText Error;
	if (!Validate(Template, Assignments, Error))
	{
		Result.Failures.Add(Error);
		return Result;
	}

	const int32 Rows = Assignments[0].Num();
	FScopedSlowTask Progress(Rows, LOCTEXT("BatchProgress", "Baking AnimStitch batch..."));
	Progress.MakeDialog(true);

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		if (Progress.ShouldCancel())
		{
			Result.Failures.Add(LOCTEXT("BatchCancelled", "Cancelled by user."));
			break;
		}
		TArray<UAnimSequence*> RowClips;
		for (const TArray<UAnimSequence*>& List : Assignments)
		{
			RowClips.Add(List[Row]);
		}
		const FString Name = MakeOutputName(Options, RowClips);
		Progress.EnterProgressFrame(1.f, FText::FromString(Name));

		const FString Path = Options.OutputFolder / Name;
		FString BakeError;
		const FAnimStitchTimeline RowTimeline = BuildRowTimeline(Template, RowClips, Options.bFullLengthForBatchSlots);
		FAnimStitchBakeOptions BakeOptions;
		BakeOptions.FrameRate = Options.FrameRate > 0 ? FFrameRate(Options.FrameRate, 1) : FFrameRate(0, 1);
		BakeOptions.bSavePackage = Options.bSavePackages;
		BakeOptions.bOverwriteExisting = Options.bOverwriteExisting;
		if (FAnimStitchBaker::Bake(RowTimeline, Path, BakeOptions, BakeError))
		{
			Result.CreatedAssets.Add(Path);
		}
		else
		{
			Result.Failures.Add(FText::Format(LOCTEXT("BatchRowFailed", "{0}: {1}"), FText::FromString(Name), FText::FromString(BakeError)));
		}
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE



