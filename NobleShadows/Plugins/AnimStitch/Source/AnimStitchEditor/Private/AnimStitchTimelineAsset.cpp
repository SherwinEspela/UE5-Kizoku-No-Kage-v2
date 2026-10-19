#include "AnimStitchTimelineAsset.h"
#include "AnimStitchSettings.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/Skeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

UAnimStitchTimelineAsset::UAnimStitchTimelineAsset()
{
	SetFlags(RF_Transactional);
}

void UAnimStitchTimelineAsset::PostTransacted(const FTransactionObjectEvent& TransactionEvent)
{
	Super::PostTransacted(TransactionEvent);
	++EditCounter;
}

void UAnimStitchTimelineAsset::GetPlaybackRange(double& OutStart, double& OutEnd) const
{
	const double Total = Timeline.GetTotalLength();
	OutEnd = PlaybackEnd < 0.0 ? Total : FMath::Min(PlaybackEnd, Total);
	OutStart = FMath::Clamp(PlaybackStart, 0.0, FMath::Max(0.0, OutEnd - FAnimStitchTimeline::MinClipDuration));
}

void UAnimStitchTimelineAsset::SetPlaybackRange(double NewStart, double NewEnd)
{
	const double Total = Timeline.GetTotalLength();
	NewEnd = FMath::Clamp(NewEnd, FAnimStitchTimeline::MinClipDuration, Total);
	NewStart = FMath::Clamp(NewStart, 0.0, FMath::Max(0.0, NewEnd - FAnimStitchTimeline::MinClipDuration));
	Modify(); // not Touch(): the range doesn't change what the preview evaluates
	PlaybackStart = NewStart;
	PlaybackEnd = NewEnd >= Total - KINDA_SMALL_NUMBER ? -1.0 : NewEnd; // full end keeps following the timeline
}

USkeleton* UAnimStitchTimelineAsset::GetSkeleton() const
{
	return Timeline.Clips.Num() > 0 && Timeline.Clips[0].Sequence ? Timeline.Clips[0].Sequence->GetSkeleton() : nullptr;
}

int32 UAnimStitchTimelineAsset::AddSequences(const TArray<UAnimSequence*>& Sequences, int32 InsertIndex, TArray<FText>& OutWarnings)
{
	Touch();
	USkeleton* Skeleton = GetSkeleton();
	int32 Index = (InsertIndex == INDEX_NONE) ? Timeline.Clips.Num() : FMath::Clamp(InsertIndex, 0, Timeline.Clips.Num());
	int32 Added = 0;
	for (UAnimSequence* Seq : Sequences)
	{
		if (!Seq || !Seq->GetSkeleton() || !Seq->GetDataModel())
		{
			OutWarnings.Add(FText::Format(LOCTEXT("WarnInvalid", "'{0}' has no skeleton or animation data and was skipped."), FText::FromString(GetNameSafe(Seq))));
			continue;
		}
		if (!Skeleton)
		{
			Skeleton = Seq->GetSkeleton();
		}
		else if (Seq->GetSkeleton() != Skeleton)
		{
			OutWarnings.Add(FText::Format(LOCTEXT("WarnSkeleton", "'{0}' uses skeleton '{1}', but this timeline uses '{2}'. It was skipped."),
				FText::FromString(Seq->GetName()), FText::FromString(Seq->GetSkeleton()->GetName()), FText::FromString(Skeleton->GetName())));
			continue;
		}
		FAnimStitchClip Clip;
		Clip.Sequence = Seq;
		Clip.TrimIn = 0.0;
		Clip.TrimOut = Seq->GetPlayLength();
		Clip.BlendIn = GetDefault<UAnimStitchSettings>()->DefaultCrossfadeCurve;
		Timeline.Clips.Insert(Clip, Index++);
		++Added;
	}
	Timeline.Relayout();
	return Added;
}

void UAnimStitchTimelineAsset::RemoveClip(int32 Index)
{
	if (!Timeline.Clips.IsValidIndex(Index))
	{
		return;
	}
	Touch();
	Timeline.Clips.RemoveAt(Index);
	if (Timeline.RootAuthorityIndex > Index || Timeline.RootAuthorityIndex >= Timeline.Clips.Num())
	{
		Timeline.RootAuthorityIndex = FMath::Max(0, Timeline.RootAuthorityIndex - 1);
	}
	if (Timeline.Clips.IsValidIndex(Index))
	{
		Timeline.Clips[Index].OverlapIn = 0.0;
	}
	Timeline.Relayout();
}

int32 UAnimStitchTimelineAsset::DuplicateClip(int32 Index)
{
	if (!Timeline.Clips.IsValidIndex(Index))
	{
		return INDEX_NONE;
	}
	Touch();
	FAnimStitchClip Copy = Timeline.Clips[Index];
	Copy.OverlapIn = 0.0;
	Timeline.Clips.Insert(Copy, Index + 1);
	if (Timeline.RootAuthorityIndex > Index)
	{
		++Timeline.RootAuthorityIndex;
	}
	if (Timeline.Clips.IsValidIndex(Index + 2))
	{
		Timeline.Clips[Index + 2].OverlapIn = 0.0;
	}
	Timeline.Relayout();
	return Index + 1;
}

void UAnimStitchTimelineAsset::MoveClip(int32 From, int32 To)
{
	if (From == To || !Timeline.Clips.IsValidIndex(From) || !Timeline.Clips.IsValidIndex(To))
	{
		return;
	}
	Touch();
	const int32 Lo = FMath::Min(From, To);
	const int32 Hi = FMath::Max(From, To);
	int32& Auth = Timeline.RootAuthorityIndex;
	if (Auth == From)
	{
		Auth = To;
	}
	else if (From < To && Auth > From && Auth <= To)
	{
		--Auth;
	}
	else if (From > To && Auth >= To && Auth < From)
	{
		++Auth;
	}
	FAnimStitchClip Moved = Timeline.Clips[From];
	Timeline.Clips.RemoveAt(From);
	Timeline.Clips.Insert(Moved, To);
	for (int32 i = Lo; i <= FMath::Min(Hi + 1, Timeline.Clips.Num() - 1); ++i)
	{
		Timeline.Clips[i].OverlapIn = 0.0;
	}
	Timeline.Relayout();
}

void UAnimStitchTimelineAsset::SetOverlap(int32 Index, double NewOverlap)
{
	if (Index <= 0 || !Timeline.Clips.IsValidIndex(Index))
	{
		return;
	}
	Touch();
	Timeline.Clips[Index].OverlapIn = NewOverlap;
	Timeline.Relayout();
}

void UAnimStitchTimelineAsset::SetTrim(int32 Index, double NewTrimIn, double NewTrimOut)
{
	if (!Timeline.Clips.IsValidIndex(Index) || !Timeline.Clips[Index].Sequence)
	{
		return;
	}
	Touch();
	FAnimStitchClip& Clip = Timeline.Clips[Index];
	const double Length = Clip.Sequence->GetPlayLength();
	NewTrimIn = FMath::Clamp(NewTrimIn, 0.0, Length - FAnimStitchTimeline::MinClipDuration);
	NewTrimOut = FMath::Clamp(NewTrimOut, NewTrimIn + FAnimStitchTimeline::MinClipDuration, Length);
	Clip.TrimIn = NewTrimIn;
	Clip.TrimOut = NewTrimOut;
	Timeline.Relayout();
}

void UAnimStitchTimelineAsset::SetPredictFeet(bool bPredict)
{
	Touch();
	Timeline.bPredictFeet = bPredict;
}

void UAnimStitchTimelineAsset::SetFootLockSettings(const FAnimStitchFootLockSettings& Settings)
{
	Touch();
	Timeline.FootLock = Settings;
}

namespace
{
	double FootEventTime(const FAnimStitchClip& Clip, const FAnimStitchFootEvent& Event)
	{
		return Clip.StartTime + (Event.SrcTime - Clip.TrimIn);
	}

	void RemoveFootEventsIn(FAnimStitchTimeline& Timeline, int32 Foot, double T0, double T1)
	{
		for (FAnimStitchClip& Clip : Timeline.Clips)
		{
			Clip.FootEvents.RemoveAll([&](const FAnimStitchFootEvent& E)
			{
				const double T = FootEventTime(Clip, E);
				return E.Foot == Foot && T >= T0 && T <= T1;
			});
		}
	}

	void StoreFootEvent(FAnimStitchTimeline& Timeline, int32 Foot, double Time, EAnimStitchFootAction Action)
	{
		if (Timeline.Clips.Num() == 0)
		{
			return;
		}
		// The topmost clip containing the time owns the event; outside every clip, the nearest end clip does.
		int32 Owner = Time < Timeline.Clips[0].StartTime ? 0 : Timeline.Clips.Num() - 1;
		for (int32 i = Timeline.Clips.Num() - 1; i >= 0; --i)
		{
			if (Time >= Timeline.Clips[i].StartTime && Time <= Timeline.Clips[i].GetEndTime())
			{
				Owner = i;
				break;
			}
		}
		FAnimStitchFootEvent Event;
		Event.Foot = Foot;
		Event.SrcTime = Timeline.Clips[Owner].TrimIn + (Time - Timeline.Clips[Owner].StartTime);
		Event.Action = Action;
		Timeline.Clips[Owner].FootEvents.Add(Event);
	}
}

void UAnimStitchTimelineAsset::AddFootEvent(int32 Foot, double TimelineTime, EAnimStitchFootAction Action)
{
	Touch();
	constexpr double SameMoment = 1.0 / 120.0;
	RemoveFootEventsIn(Timeline, Foot, TimelineTime - SameMoment, TimelineTime + SameMoment);
	StoreFootEvent(Timeline, Foot, TimelineTime, Action);
}

void UAnimStitchTimelineAsset::PaintFootLock(int32 Foot, double TimelineStart, double TimelineEnd)
{
	if (TimelineEnd < TimelineStart)
	{
		Swap(TimelineStart, TimelineEnd);
	}
	Touch();
	RemoveFootEventsIn(Timeline, Foot, TimelineStart, TimelineEnd);
	StoreFootEvent(Timeline, Foot, TimelineStart, EAnimStitchFootAction::Lock);
	StoreFootEvent(Timeline, Foot, TimelineEnd, EAnimStitchFootAction::Unlock);
}

void UAnimStitchTimelineAsset::ClearFootEvents(int32 Foot)
{
	Touch();
	for (FAnimStitchClip& Clip : Timeline.Clips)
	{
		Clip.FootEvents.RemoveAll([Foot](const FAnimStitchFootEvent& E) { return Foot == INDEX_NONE || E.Foot == Foot; });
	}
}
void UAnimStitchTimelineAsset::AddDefaultCrossfade(int32 Index)
{
	if (Index > 0 && Timeline.Clips.IsValidIndex(Index))
	{
		SetOverlap(Index, GetDefault<UAnimStitchSettings>()->DefaultCrossfadeLength);
		Timeline.Clips[Index].BlendIn = GetDefault<UAnimStitchSettings>()->DefaultCrossfadeCurve;
	}
}

void UAnimStitchTimelineAsset::SetBatchSlot(int32 Index, bool bBatchSlot)
{
	if (Timeline.Clips.IsValidIndex(Index))
	{
		Touch();
		Timeline.Clips[Index].bBatchSlot = bBatchSlot;
	}
}

void UAnimStitchTimelineAsset::SetRootAuthority(int32 Index)
{
	if (Timeline.Clips.IsValidIndex(Index))
	{
		Touch();
		Timeline.RootAuthorityIndex = Index;
	}
}

bool UAnimStitchTimelineAsset::CanCutClipAt(int32 Index, double TimelineTime) const
{
	if (!Timeline.Clips.IsValidIndex(Index))
	{
		return false;
	}
	const FAnimStitchClip& Clip = Timeline.Clips[Index];
	return TimelineTime > Clip.StartTime + FAnimStitchTimeline::MinClipDuration
		&& TimelineTime < Clip.GetEndTime() - FAnimStitchTimeline::MinClipDuration;
}

bool UAnimStitchTimelineAsset::SplitClip(int32 Index, double TimelineTime)
{
	if (!CanCutClipAt(Index, TimelineTime))
	{
		return false;
	}
	Touch();
	FAnimStitchClip Second = Timeline.Clips[Index];
	const double SourceTime = Second.TrimIn + (TimelineTime - Second.StartTime);
	Second.TrimIn = SourceTime;
	Second.OverlapIn = 0.0; // butt-joined to the first half
	Timeline.Clips[Index].TrimOut = SourceTime;
	Timeline.Clips.Insert(Second, Index + 1);
	if (Timeline.RootAuthorityIndex > Index)
	{
		++Timeline.RootAuthorityIndex;
	}
	Timeline.Relayout();
	return true;
}

bool UAnimStitchTimelineAsset::TrimClipStartTo(int32 Index, double TimelineTime)
{
	if (!CanCutClipAt(Index, TimelineTime))
	{
		return false;
	}
	const FAnimStitchClip& Clip = Timeline.Clips[Index];
	SetTrim(Index, Clip.TrimIn + (TimelineTime - Clip.StartTime), Clip.TrimOut);
	return true;
}

bool UAnimStitchTimelineAsset::TrimClipEndTo(int32 Index, double TimelineTime)
{
	if (!CanCutClipAt(Index, TimelineTime))
	{
		return false;
	}
	const FAnimStitchClip& Clip = Timeline.Clips[Index];
	SetTrim(Index, Clip.TrimIn, Clip.TrimIn + (TimelineTime - Clip.StartTime));
	return true;
}

void UAnimStitchTimelineAsset::SetBlendCurve(int32 Index, EAnimStitchBlendCurve Curve)
{
	if (Index <= 0 || !Timeline.Clips.IsValidIndex(Index))
	{
		return;
	}
	Touch();
	Timeline.Clips[Index].BlendIn = Curve;
}

#undef LOCTEXT_NAMESPACE



