#include "AnimStitchTimelineAsset.h"
#include "AnimStitchSettings.h"
#include "AnimStitchManualEdits.h"
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
	bUnsavedChanges = true;
	++EditCounter;
	++ManualCounter;
}

UAnimStitchTimelineAsset::FKeyFollowScope::FKeyFollowScope(UAnimStitchTimelineAsset& InAsset)
	: Asset(InAsset)
{
	FAnimStitchTimeline& TL = Asset.Timeline;
	TSet<FGuid> Seen;
	for (FAnimStitchClip& Clip : TL.Clips)
	{
		if (!Clip.ClipId.IsValid() || Seen.Contains(Clip.ClipId)) { Clip.ClipId = FGuid::NewGuid(); }
		Seen.Add(Clip.ClipId);
	}
	constexpr double Eps = 1e-6;
	for (int32 t = 0; t < TL.Tracks.Num(); ++t)
	{
		for (int32 k = 0; k < TL.Tracks[t].Keys.Num(); ++k)
		{
			const double Time = TL.Tracks[t].Keys[k].Time;
			FOwner Owner;
			Owner.Track = t;
			Owner.Key = k;
			for (int32 c = TL.Clips.Num() - 1; c >= 0; --c) // topmost clip wins inside a crossfade
			{
				const FAnimStitchClip& Clip = TL.Clips[c];
				if (Time >= Clip.StartTime - Eps && Time <= Clip.GetEndTime() + Eps)
				{
					Owner.Clip = Clip.ClipId;
					Owner.SourceTime = Clip.TrimIn + (Time - Clip.StartTime);
					break;
				}
			}
			Owners.Add(Owner);
		}
	}
}

void UAnimStitchTimelineAsset::FKeyFollowScope::Apply()
{
	if (bApplied) { return; }
	bApplied = true;
	FAnimStitchTimeline& TL = Asset.Timeline;
	bool bChanged = false;
	for (int32 t = 0; t < TL.Tracks.Num(); ++t)
	{
		FAnimStitchTrack& Track = TL.Tracks[t];
		TArray<TPair<double, FAnimStitchTrackKey>> Moved;
		for (const FOwner& Owner : Owners)
		{
			if (Owner.Track != t || !Track.Keys.IsValidIndex(Owner.Key)) { continue; }
			FAnimStitchTrackKey Key = Track.Keys[Owner.Key];
			if (Owner.Clip.IsValid())
			{
				if (const FAnimStitchClip* Clip = TL.Clips.FindByPredicate([&Owner](const FAnimStitchClip& C) { return C.ClipId == Owner.Clip; }))
				{
					const double NewTime = FMath::Clamp(Clip->StartTime + (Owner.SourceTime - Clip->TrimIn), Clip->StartTime, Clip->GetEndTime());
					if (!FMath::IsNearlyEqual(NewTime, Key.Time, 1e-9)) { bChanged = true; }
					Key.Time = NewTime;
				}
			}
			Moved.Add({ Key.Time, Key });
		}
		if (Moved.Num() != Track.Keys.Num()) { continue; } // the key list changed under us: leave it alone
		Moved.StableSort([](const TPair<double, FAnimStitchTrackKey>& A, const TPair<double, FAnimStitchTrackKey>& B) { return A.Key < B.Key; });
		Track.Keys.Reset();
		for (const TPair<double, FAnimStitchTrackKey>& Entry : Moved)
		{
			if (Track.Keys.Num() > 0 && FMath::Abs(Track.Keys.Last().Time - Entry.Key) < 1e-4) { Track.Keys.Last() = Entry.Value; continue; } // two keys landed on the same moment
			Track.Keys.Add(Entry.Value);
		}
	}
	if (bChanged) { Asset.TouchManual(); }
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
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
	USkeleton* Skeleton = GetSkeleton();
	int32 Index = (InsertIndex == INDEX_NONE) ? Timeline.Clips.Num() : FMath::Clamp(InsertIndex, 0, Timeline.Clips.Num());
	int32 Added = 0;
	const int32 FirstIndex = Index;
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
		Clip.ClipId = FGuid::NewGuid();
		Clip.TrimIn = 0.0;
		Clip.TrimOut = Seq->GetPlayLength();
		Clip.BlendIn = GetDefault<UAnimStitchSettings>()->DefaultCrossfadeCurve;
		Timeline.Clips.Insert(Clip, Index++);
		++Added;
	}
	Timeline.Relayout();
	KeyFollow.Apply(); // existing keys first, then the new clips get their end keys
	if (Added > 0 && Timeline.Tracks.Num() > 0)
	{
		// Auto-key: every correction lane gets a key at the end of each new clip (holding its value there).
		TouchManual();
		for (int32 c = FirstIndex; c < FirstIndex + Added && c < Timeline.Clips.Num(); ++c)
		{
			const double EndTime = Timeline.Clips[c].GetEndTime();
			for (FAnimStitchTrack& Track : Timeline.Tracks)
			{
				if (Track.Keys.Num() == 0 || FAnimStitchManualEdits::FindKey(Track, EndTime) != INDEX_NONE) { continue; }
				FAnimStitchTrackKey Key = FAnimStitchManualEdits::Sample(Track, EndTime);
				Key.Time = EndTime;
				FAnimStitchManualEdits::SetKey(Track, Key);
			}
		}
	}
	return Added;
}

void UAnimStitchTimelineAsset::RemoveClip(int32 Index)
{
	if (!Timeline.Clips.IsValidIndex(Index))
	{
		return;
	}
	Touch();
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
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
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
	FAnimStitchClip Copy = Timeline.Clips[Index];
	Copy.ClipId = FGuid::NewGuid();
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
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
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
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
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
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
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

	FAnimStitchFootEvent* StoreFootEvent(FAnimStitchTimeline& Timeline, int32 Foot, double Time, EAnimStitchFootAction Action)
	{
		if (Timeline.Clips.Num() == 0)
		{
			return nullptr;
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
		const int32 Added = Timeline.Clips[Owner].FootEvents.Add(Event);
		return &Timeline.Clips[Owner].FootEvents[Added];
	}
}

void UAnimStitchTimelineAsset::AddFootEvent(int32 Foot, double TimelineTime, EAnimStitchFootAction Action)
{
	Touch();
	constexpr double SameMoment = 1.0 / 120.0;
	RemoveFootEventsIn(Timeline, Foot, TimelineTime - SameMoment, TimelineTime + SameMoment);
	StoreFootEvent(Timeline, Foot, TimelineTime, Action);
}

void UAnimStitchTimelineAsset::MoveFootEvent(int32 Foot, double FromTime, double ToTime)
{
	constexpr double SameMoment = 1.0 / 120.0;
	for (FAnimStitchClip& Clip : Timeline.Clips)
	{
		for (int32 i = 0; i < Clip.FootEvents.Num(); ++i)
		{
			const FAnimStitchFootEvent Event = Clip.FootEvents[i];
			if (Event.Foot != Foot || FMath::Abs(FootEventTime(Clip, Event) - FromTime) > SameMoment) { continue; }
			Touch();
			Clip.FootEvents.RemoveAt(i);
			if (FAnimStitchFootEvent* Stored = StoreFootEvent(Timeline, Foot, ToTime, Event.Action)) { Stored->PinOffset = Event.PinOffset; }
			return;
		}
	}
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
int32 UAnimStitchTimelineAsset::AddTrack(const FAnimStitchTrack& Track)
{
	for (int32 i = 0; i < Timeline.Tracks.Num(); ++i)
	{
		if (Timeline.Tracks[i].Kind == Track.Kind && Timeline.Tracks[i].Bone == Track.Bone)
		{
			return i;
		}
	}
	TouchManual();
	FAnimStitchTrack NewTrack = Track;
	NewTrack.bKeepFeetPlanted = Track.Kind == EAnimStitchTrackKind::Bone && FAnimStitchManualEdits::IsPelvisBone(Track.Bone) && Track.bKeepFeetPlanted;
	if (NewTrack.Keys.Num() == 0 && NewTrack.Kind != EAnimStitchTrackKind::IKPole)
	{
		// Like a new Sequencer section: keys at the start and the end of the timeline (identity offset / IK off).
		FAnimStitchTrackKey Start;
		Start.Time = 0.0;
		Start.Weight = NewTrack.Kind == EAnimStitchTrackKind::IKLimb ? 0.f : 1.f;
		FAnimStitchManualEdits::SetKey(NewTrack, Start);
		const double Total = Timeline.GetTotalLength();
		if (Total > 0.05)
		{
			FAnimStitchTrackKey End = Start;
			End.Time = Total;
			FAnimStitchManualEdits::SetKey(NewTrack, End);
		}
	}
	return Timeline.Tracks.Add(NewTrack);
}

void UAnimStitchTimelineAsset::RemoveTrack(int32 Index)
{
	if (Timeline.Tracks.IsValidIndex(Index))
	{
		TouchManual();
		// Removing an IK limb takes its pole lane with it.
		if (Timeline.Tracks[Index].Kind == EAnimStitchTrackKind::IKLimb)
		{
			const FName Bone = Timeline.Tracks[Index].Bone;
			for (int32 i = Timeline.Tracks.Num() - 1; i >= 0; --i)
			{
				if (i != Index && Timeline.Tracks[i].Kind == EAnimStitchTrackKind::IKPole && Timeline.Tracks[i].Bone == Bone)
				{
					Timeline.Tracks.RemoveAt(i);
					if (i < Index) { --Index; }
				}
			}
		}
		Timeline.Tracks.RemoveAt(Index);
	}
}

void UAnimStitchTimelineAsset::SetTrackKey(int32 TrackIndex, const FAnimStitchTrackKey& Key)
{
	if (!Timeline.Tracks.IsValidIndex(TrackIndex))
	{
		return;
	}
	TouchManual();
	FAnimStitchTrack& Track = Timeline.Tracks[TrackIndex];
	if (Track.Keys.Num() == 0 && Key.Time > 1e-3)
	{
		// Anchor so the correction does not apply to the whole timeline before the first key (Sequencer holds the first value).
		FAnimStitchTrackKey Anchor;
		if (Track.Kind == EAnimStitchTrackKind::Bone)
		{
			Anchor.Time = 0.0; // identity offset
		}
		else
		{
			Anchor = Key;
			Anchor.Time = FMath::Max(0.0, Key.Time - 0.1);
			Anchor.Weight = 0.f; // IK off, then ramping on
		}
		FAnimStitchManualEdits::SetKey(Track, Anchor);
	}
	FAnimStitchManualEdits::SetKey(Track, Key);
}

void UAnimStitchTimelineAsset::RemoveTrackKey(int32 TrackIndex, int32 KeyIndex)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex) && Timeline.Tracks[TrackIndex].Keys.IsValidIndex(KeyIndex))
	{
		TouchManual();
		Timeline.Tracks[TrackIndex].Keys.RemoveAt(KeyIndex);
	}
}

int32 UAnimStitchTimelineAsset::MoveTrackKey(int32 TrackIndex, int32 KeyIndex, double NewTime)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex) && Timeline.Tracks[TrackIndex].Keys.IsValidIndex(KeyIndex))
	{
		TouchManual();
		FAnimStitchTrack& Track = Timeline.Tracks[TrackIndex];
		FAnimStitchTrackKey Moved = Track.Keys[KeyIndex];
		Track.Keys.RemoveAt(KeyIndex);
		Moved.Time = FMath::Max(0.0, NewTime);
		return FAnimStitchManualEdits::SetKey(Track, Moved);
	}
	return KeyIndex;
}

void UAnimStitchTimelineAsset::SetTrackKeyWeight(int32 TrackIndex, int32 KeyIndex, float Weight)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex) && Timeline.Tracks[TrackIndex].Keys.IsValidIndex(KeyIndex))
	{
		TouchManual();
		Timeline.Tracks[TrackIndex].Keys[KeyIndex].Weight = FMath::Clamp(Weight, 0.f, 1.f);
	}
}

void UAnimStitchTimelineAsset::SetTrackControlStyle(int32 TrackIndex, bool bPole, const FAnimStitchControlStyle& Style)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex))
	{
		TouchManual();
		(bPole ? Timeline.Tracks[TrackIndex].PoleStyle : Timeline.Tracks[TrackIndex].ControlStyle) = Style;
	}
}

void UAnimStitchTimelineAsset::SetPreviewAttachmentVisible(int32 Index, bool bVisible)
{
	if (Timeline.PreviewAttachments.IsValidIndex(Index))
	{
		TouchManual();
		Timeline.PreviewAttachments[Index].bVisible = bVisible;
	}
}

void UAnimStitchTimelineAsset::SetPreviewAttachmentScale(int32 Index, const FVector& Scale)
{
	if (Timeline.PreviewAttachments.IsValidIndex(Index))
	{
		TouchManual();
		Timeline.PreviewAttachments[Index].Scale = Scale;
	}
}

void UAnimStitchTimelineAsset::SetTrackKeepFeet(int32 TrackIndex, bool bKeep)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex))
	{
		TouchManual();
		Timeline.Tracks[TrackIndex].bKeepFeetPlanted = bKeep;
	}
}

void UAnimStitchTimelineAsset::SetPinOffset(int32 ClipIndex, int32 EventIndex, const FVector& Offset)
{
	if (Timeline.Clips.IsValidIndex(ClipIndex) && Timeline.Clips[ClipIndex].FootEvents.IsValidIndex(EventIndex))
	{
		Touch(); // foot lock is an automatic pass: it has to be re-run
		Timeline.Clips[ClipIndex].FootEvents[EventIndex].PinOffset = Offset;
	}
}

void UAnimStitchTimelineAsset::AddPreviewAttachment(const FAnimStitchPreviewAttachment& Attachment)
{
	TouchManual();
	Timeline.PreviewAttachments.Add(Attachment);
}

void UAnimStitchTimelineAsset::RemovePreviewAttachment(int32 Index)
{
	if (Timeline.PreviewAttachments.IsValidIndex(Index))
	{
		TouchManual();
		Timeline.PreviewAttachments.RemoveAt(Index);
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
	Second.ClipId = FGuid::NewGuid();
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






