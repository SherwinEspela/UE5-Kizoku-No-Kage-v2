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
	// Absolute tracks: remember the spans the user emptied by deleting keys, so editing the timeline never re-fills them from the clip.
	{
		double GuardRate = 30.0;
		if (const UAnimSequence* Seq = TL.Clips.Num() > 0 ? TL.Clips[0].Sequence.Get() : nullptr) { GuardRate = Seq->GetDataModel()->GetFrameRate().AsDecimal(); }
		if (GuardRate < 1.0 || GuardRate > 120.0) { GuardRate = 30.0; }
		const double GapLimit = 1.5 / GuardRate;
		const double Total = TL.GetTotalLength();
		for (int32 t = 0; t < TL.Tracks.Num(); ++t)
		{
			const FAnimStitchTrack& Track = TL.Tracks[t];
			if (Track.Kind != EAnimStitchTrackKind::Bone || Track.Mode != EAnimStitchTrackMode::Absolute || Track.Keys.Num() == 0) { continue; }
			if (Track.Keys[0].Time > GapLimit) { FGuard G; G.Track = t; G.KeyB = 0; Guards.Add(G); }
			for (int32 k = 0; k + 1 < Track.Keys.Num(); ++k)
			{
				if (Track.Keys[k + 1].Time - Track.Keys[k].Time > GapLimit) { FGuard G; G.Track = t; G.KeyA = k; G.KeyB = k + 1; Guards.Add(G); }
			}
			if (Total - Track.Keys.Last().Time > GapLimit && TL.Clips.Num() > 0)
			{
				FGuard G; G.Track = t; G.KeyA = Track.Keys.Num() - 1; G.EndClip = TL.Clips.Last().ClipId; G.EndSourceTime = TL.Clips.Last().TrimOut; Guards.Add(G);
			}
		}
	}
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
Owner.SourceTime = Clip.SourceTimeAt(Time);
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
	Protected.SetNum(TL.Tracks.Num());
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
const double NewTime = FMath::Clamp(Clip->TimelineTimeAt(Owner.SourceTime), Clip->StartTime, Clip->GetEndTime());
					if (!FMath::IsNearlyEqual(NewTime, Key.Time, 1e-9)) { bChanged = true; }
					Key.Time = NewTime;
				}
			}
			Moved.Add({ Key.Time, Key });
		}
		if (Moved.Num() != Track.Keys.Num()) { continue; } // the key list changed under us: leave it alone
		// Where the keys that bound an emptied span ended up (Moved is still in the old key order here).
		for (const FGuard& G : Guards)
		{
			if (G.Track != t) { continue; }
			const double From = Moved.IsValidIndex(G.KeyA) ? Moved[G.KeyA].Key : -TNumericLimits<double>::Max();
			double To = TNumericLimits<double>::Max();
			if (Moved.IsValidIndex(G.KeyB)) { To = Moved[G.KeyB].Key; }
			else if (G.EndClip.IsValid())
			{
				const FAnimStitchClip* EndClip = TL.Clips.FindByPredicate([&G](const FAnimStitchClip& C) { return C.ClipId == G.EndClip; });
				To = EndClip ? FMath::Min(EndClip->TimelineTimeAt(G.EndSourceTime), EndClip->GetEndTime()) : TL.GetTotalLength();
			}
			Protected[t].Add({ From, To });
		}
		Moved.StableSort([](const TPair<double, FAnimStitchTrackKey>& A, const TPair<double, FAnimStitchTrackKey>& B) { return A.Key < B.Key; });
		Track.Keys.Reset();
		for (const TPair<double, FAnimStitchTrackKey>& Entry : Moved)
		{
			if (Track.Keys.Num() > 0 && FMath::Abs(Track.Keys.Last().Time - Entry.Key) < 1e-4)
			{
				// Two keys landed on the same moment: merge their channels (the later one wins where both hold a channel).
				FAnimStitchTrackKey& Last = Track.Keys.Last();
				const uint8 Mask = Entry.Value.Channels;
				const uint8 OldMask = Last.Channels;
				if (Mask == AnimStitchChannel::All) { Last = Entry.Value; }
				else
				{
					if (Mask & AnimStitchChannel::Translation) { Last.Translation = Entry.Value.Translation; }
					if (Mask & AnimStitchChannel::Rotation) { Last.Rotation = Entry.Value.Rotation; }
					if (Mask & AnimStitchChannel::Scale) { Last.Scale = Entry.Value.Scale; }
					Last.Channels = OldMask | Mask;
				}
				continue;
			}
			Track.Keys.Add(Entry.Value);
		}
	}
	if (bChanged)
	{
		if (Asset.Timeline.Tracks.ContainsByPredicate([](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::RootOffset; })) { Asset.Touch(); ++Asset.ManualCounter; }
		else { Asset.TouchManual(); }
	}
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
	bUnsavedChanges = true; // but it is part of the project
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
	FillAbsoluteTracks(&KeyFollow); // absolute tracks get per-frame keys over the new span
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

void UAnimStitchTimelineAsset::RemoveClips(const TArray<int32>& Indices)
{
	TArray<int32> Sorted = Indices;
	Sorted.Sort([](int32 A, int32 B) { return A > B; });
	int32 Last = INDEX_NONE;
	for (const int32 Index : Sorted)
	{
		if (Index == Last) { continue; }
		Last = Index;
		RemoveClip(Index); // highest index first, so the rest stay valid
	}
}

void UAnimStitchTimelineAsset::EnsureClipIds()
{
	TSet<FGuid> Seen;
	for (FAnimStitchClip& Clip : Timeline.Clips)
	{
		if (!Clip.ClipId.IsValid() || Seen.Contains(Clip.ClipId)) { Clip.ClipId = FGuid::NewGuid(); }
		Seen.Add(Clip.ClipId);
	}
}

void UAnimStitchTimelineAsset::MoveClipBlock(const TArray<int32>& Indices, int32 TargetAmongRemaining)
{
	TArray<int32> Sorted;
	for (const int32 Index : Indices) { if (Timeline.Clips.IsValidIndex(Index)) { Sorted.AddUnique(Index); } }
	Sorted.Sort();
	if (Sorted.Num() == 0 || Sorted.Num() >= Timeline.Clips.Num()) { return; }
	Touch();
	FKeyFollowScope KeyFollow(*this); // also gives every clip a unique id; correction keys follow their clips
	const FGuid AuthorityId = Timeline.Clips.IsValidIndex(Timeline.RootAuthorityIndex) ? Timeline.Clips[Timeline.RootAuthorityIndex].ClipId : FGuid();
	TArray<FAnimStitchClip> Block, Rest;
	for (int32 i = 0; i < Timeline.Clips.Num(); ++i)
	{
		(Sorted.Contains(i) ? Block : Rest).Add(Timeline.Clips[i]);
	}
	const int32 Target = FMath::Clamp(TargetAmongRemaining, 0, Rest.Num());
	const int32 Lo = FMath::Min(Sorted[0], Target);
	const int32 Hi = FMath::Max(Sorted.Last(), Target + Block.Num() - 1);
	Rest.Insert(Block, Target);
	Timeline.Clips = MoveTemp(Rest);
	for (int32 i = Lo; i <= FMath::Min(Hi + 1, Timeline.Clips.Num() - 1); ++i)
	{
		Timeline.Clips[i].OverlapIn = 0.0;
	}
	const int32 NewAuthority = Timeline.Clips.IndexOfByPredicate([&AuthorityId](const FAnimStitchClip& C) { return C.ClipId == AuthorityId; });
	Timeline.RootAuthorityIndex = NewAuthority != INDEX_NONE ? NewAuthority : 0;
	Timeline.Relayout();
	KeyFollow.Apply();
	FillAbsoluteTracks(&KeyFollow);
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
	KeyFollow.Apply();
	FillAbsoluteTracks(&KeyFollow); // the copy adds timeline span
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
	KeyFollow.Apply();
	FillAbsoluteTracks(&KeyFollow); // a smaller overlap makes the timeline longer
}

void UAnimStitchTimelineAsset::SetClipRate(int32 Index, double NewRate)
{
	if (!Timeline.Clips.IsValidIndex(Index))
	{
		return;
	}
	NewRate = FMath::Clamp(NewRate, FAnimStitchClip::MinPlayRate, FAnimStitchClip::MaxPlayRate);
	if (FMath::IsNearlyEqual(Timeline.Clips[Index].PlayRate, NewRate, 1e-9)) { return; }
	Touch();
	FKeyFollowScope KeyFollow(*this); // correction keys follow their clip
	Timeline.Clips[Index].PlayRate = NewRate;
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
	KeyFollow.Apply();
	FillAbsoluteTracks(&KeyFollow); // an extended clip adds timeline span
}

void UAnimStitchTimelineAsset::SetGroundFeet(bool bGround)
{
	Touch();
	Timeline.bGroundFeet = bGround;
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
return Clip.TimelineTimeAt(Event.SrcTime);
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
Event.SrcTime = Timeline.Clips[Owner].SourceTimeAt(Time);
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
bool UAnimStitchTimelineAsset::CaptureBlendedBoneTransform(FName Bone, double Time, FAnimStitchTrackKey& InOutKey) const
{
	FAnimStitchEvaluator Evaluator;
	FString Error;
	if (!Evaluator.Initialize(Timeline, Error)) { return false; }
	const FReferenceSkeleton& Ref = Evaluator.GetSkeleton()->GetReferenceSkeleton();
	const int32 BoneIndex = Ref.FindBoneIndex(Bone);
	if (BoneIndex == INDEX_NONE) { return false; }
	TArray<FTransform> Pose;
	Evaluator.EvaluatePose(Time, Pose);
	// Relative to the root bone: an absolute key travels with the root.
	const FTransform Component = FAnimStitchFootLock::GetComponentSpace(Ref, Pose, BoneIndex).GetRelativeTransform(FAnimStitchFootLock::GetComponentSpace(Ref, Pose, 0));
	InOutKey.Translation = Component.GetTranslation();
	InOutKey.Rotation = Component.GetRotation().GetNormalized();
	InOutKey.Scale = Component.GetScale3D();
	return true;
}

/** Fills an Absolute bone track with one root-relative key per frame across the whole timeline, taken from the blended (pre-manual) pose. */
static void BakeAbsoluteKeys(const FAnimStitchTimeline& Timeline, FAnimStitchTrack& Track)
{
	FAnimStitchEvaluator Evaluator;
	FString Error;
	if (!Evaluator.Initialize(Timeline, Error)) { return; }
	const FReferenceSkeleton& Ref = Evaluator.GetSkeleton()->GetReferenceSkeleton();
	const int32 BoneIndex = Ref.FindBoneIndex(Track.Bone);
	const double Total = Timeline.GetTotalLength();
	if (BoneIndex == INDEX_NONE || Total <= KINDA_SMALL_NUMBER) { return; }
	double Rate = 30.0;
	if (const UAnimSequence* Seq = Timeline.Clips.Num() > 0 ? Timeline.Clips[0].Sequence.Get() : nullptr)
	{
		Rate = Seq->GetDataModel()->GetFrameRate().AsDecimal();
	}
	if (Rate < 1.0 || Rate > 120.0) { Rate = 30.0; }
	const int32 NumFrames = FMath::Max(1, FMath::CeilToInt(Total * Rate - 1e-6));
	Track.Keys.Reserve(NumFrames + 1);
	TArray<FTransform> Pose;
	for (int32 f = 0; f <= NumFrames; ++f)
	{
		const double Time = f == NumFrames ? Total : f / Rate;
		Evaluator.EvaluatePose(Time, Pose);
		const FTransform Rel = FAnimStitchFootLock::GetComponentSpace(Ref, Pose, BoneIndex).GetRelativeTransform(FAnimStitchFootLock::GetComponentSpace(Ref, Pose, 0));
		FAnimStitchTrackKey Key;
		Key.Time = Time;
		Key.Translation = Rel.GetTranslation();
		Key.Rotation = Rel.GetRotation().GetNormalized();
		Key.Scale = Rel.GetScale3D();
		Track.Keys.Add(Key);
	}
}
void UAnimStitchTimelineAsset::FillAbsoluteTracks(const FKeyFollowScope* Scope)
{
	TArray<int32> Targets;
	for (int32 i = 0; i < Timeline.Tracks.Num(); ++i)
	{
		if (Timeline.Tracks[i].Kind == EAnimStitchTrackKind::Bone && Timeline.Tracks[i].Mode == EAnimStitchTrackMode::Absolute) { Targets.Add(i); }
	}
	const double Total = Timeline.GetTotalLength();
	if (Targets.Num() == 0 || Total <= KINDA_SMALL_NUMBER) { return; }
	double Rate = 30.0;
	if (const UAnimSequence* Seq = Timeline.Clips.Num() > 0 ? Timeline.Clips[0].Sequence.Get() : nullptr)
	{
		Rate = Seq->GetDataModel()->GetFrameRate().AsDecimal();
	}
	if (Rate < 1.0 || Rate > 120.0) { Rate = 30.0; }
	const int32 NumFrames = FMath::Max(1, FMath::CeilToInt(Total * Rate - 1e-6));
	const double Near = 0.75 / Rate; // a frame with a key this close is already covered

	TArray<TArray<double>> Missing;
	Missing.SetNum(Targets.Num());
	bool bAny = false;
	for (int32 t = 0; t < Targets.Num(); ++t)
	{
		const TArray<FAnimStitchTrackKey>& Keys = Timeline.Tracks[Targets[t]].Keys;
		int32 K = 0;
		for (int32 f = 0; f <= NumFrames; ++f)
		{
			const double Time = f == NumFrames ? Total : f / Rate;
			while (K + 1 < Keys.Num() && Keys[K + 1].Time <= Time) { ++K; }
			double Distance = TNumericLimits<double>::Max();
			if (Keys.IsValidIndex(K)) { Distance = FMath::Abs(Keys[K].Time - Time); }
			if (Keys.IsValidIndex(K + 1)) { Distance = FMath::Min(Distance, FMath::Abs(Keys[K + 1].Time - Time)); }
			bool bEmptied = false; // a span the user emptied by deleting keys interpolates; it is never refilled from the clip
				if (Scope && Scope->Protected.IsValidIndex(Targets[t]))
				{
					for (const TPair<double, double>& Range : Scope->Protected[Targets[t]]) { bEmptied |= Time > Range.Key + 1e-9 && Time < Range.Value - 1e-9; }
				}
				if (Distance > Near && !bEmptied) { Missing[t].Add(Time); bAny = true; }
		}
	}
	if (!bAny) { return; }

	FAnimStitchEvaluator Evaluator;
	FString Error;
	if (!Evaluator.Initialize(Timeline, Error)) { return; }
	const FReferenceSkeleton& Ref = Evaluator.GetSkeleton()->GetReferenceSkeleton();
	TArray<FTransform> Pose;
	TouchManual();
	for (int32 t = 0; t < Targets.Num(); ++t)
	{
		FAnimStitchTrack& Track = Timeline.Tracks[Targets[t]];
		const int32 BoneIndex = Ref.FindBoneIndex(Track.Bone);
		if (BoneIndex == INDEX_NONE) { continue; }
		for (const double Time : Missing[t])
		{
			Evaluator.EvaluatePose(Time, Pose);
			const FTransform Rel = FAnimStitchFootLock::GetComponentSpace(Ref, Pose, BoneIndex).GetRelativeTransform(FAnimStitchFootLock::GetComponentSpace(Ref, Pose, 0));
			FAnimStitchTrackKey Key;
			Key.Time = Time;
			Key.Translation = Rel.GetTranslation();
			Key.Rotation = Rel.GetRotation().GetNormalized();
			Key.Scale = Rel.GetScale3D();
			FAnimStitchManualEdits::SetKey(Track, Key);
		}
	}
}

int32 UAnimStitchTimelineAsset::AddTrack(const FAnimStitchTrack& Track)
{
	for (int32 i = 0; i < Timeline.Tracks.Num(); ++i)
	{
		// One track per bone and kind; there is only ever ONE Root offset track, whichever way it was added.
		if (Timeline.Tracks[i].Kind == Track.Kind && (Track.Kind == EAnimStitchTrackKind::RootOffset || Timeline.Tracks[i].Bone == Track.Bone))
		{
			return i;
		}
	}
	if (Track.Kind == EAnimStitchTrackKind::RootOffset) { Touch(); ++ManualCounter; } else { TouchManual(); }
	FAnimStitchTrack NewTrack = Track;
	// Control hierarchy: a new bone control is parented to the control of its skeleton parent bone, when that control exists (nothing is invented otherwise).
	if (NewTrack.Kind == EAnimStitchTrackKind::Bone && NewTrack.ParentBone.IsNone())
	{
		if (const USkeleton* Skeleton = GetSkeleton())
		{
			const FReferenceSkeleton& SkelRef = Skeleton->GetReferenceSkeleton();
			const int32 BoneIndex = SkelRef.FindBoneIndex(NewTrack.Bone);
			const int32 ParentIndex = BoneIndex != INDEX_NONE ? SkelRef.GetParentIndex(BoneIndex) : INDEX_NONE;
			if (ParentIndex != INDEX_NONE)
			{
				const FName ParentName = SkelRef.GetBoneName(ParentIndex);
				if (Timeline.Tracks.ContainsByPredicate([ParentName](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::Bone && T.Bone == ParentName; })) { NewTrack.ParentBone = ParentName; }
			}
		}
	}
	if (NewTrack.Kind != EAnimStitchTrackKind::Bone) { NewTrack.Mode = EAnimStitchTrackMode::Additive; } // Absolute is for FK / core bone tracks only
	if (NewTrack.Kind == EAnimStitchTrackKind::RootOffset)
	{
		NewTrack.bRotationOnly = false;
		NewTrack.bKeepFeetPlanted = false;
	}
	NewTrack.bKeepFeetPlanted = Track.Kind == EAnimStitchTrackKind::Bone && FAnimStitchManualEdits::IsPelvisBone(Track.Bone) && Track.bKeepFeetPlanted;
	if (NewTrack.Keys.Num() == 0 && NewTrack.Kind == EAnimStitchTrackKind::Bone && NewTrack.Mode == EAnimStitchTrackMode::Absolute)
	{
		// Absolute: bake the bone's evaluated (blended) pose into a key on EVERY frame, like baking onto a Control Rig. The track then owns the
		// bone: untouched frames reproduce the underlying animation, edits only change the keys the user touches.
		BakeAbsoluteKeys(Timeline, NewTrack);
		return Timeline.Tracks.Add(NewTrack);
	}
	if (NewTrack.Keys.Num() == 0 && NewTrack.Kind != EAnimStitchTrackKind::IKPole)
	{
		// Like a new Sequencer section: keys at the start and the end of the timeline (identity offset / IK off).
		FAnimStitchTrackKey Start;
		Start.Time = 0.0;
		Start.Weight = NewTrack.Kind == EAnimStitchTrackKind::IKLimb ? 0.f : 1.f;
		FAnimStitchTrackKey End;
		bool bHaveEndValue = false;
		FAnimStitchManualEdits::SetKey(NewTrack, Start);
		const double Total = Timeline.GetTotalLength();
		if (Total > 0.05)
		{
			if (!bHaveEndValue) { End = Start; }
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
		TouchTrack(Index);
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
	TouchTrack(TrackIndex);
	FAnimStitchTrack& Track = Timeline.Tracks[TrackIndex];
	if (Track.Keys.Num() == 0 && Key.Time > 1e-3 && Track.Mode == EAnimStitchTrackMode::Absolute && Track.Kind == EAnimStitchTrackKind::Bone)
	{
		// Absolute track without boundary keys (the blend could not be evaluated at creation): it simply starts at its first key.
	}
	else if (Track.Keys.Num() == 0 && Key.Time > 1e-3)
	{
		// Anchor so the correction does not apply to the whole timeline before the first key (Sequencer holds the first value).
		FAnimStitchTrackKey Anchor;
		if (Track.Kind == EAnimStitchTrackKind::Bone || Track.Kind == EAnimStitchTrackKind::RootOffset)
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
		TouchTrack(TrackIndex);
		Timeline.Tracks[TrackIndex].Keys.RemoveAt(KeyIndex);
	}
}

int32 UAnimStitchTimelineAsset::MoveTrackKey(int32 TrackIndex, int32 KeyIndex, double NewTime)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex) && Timeline.Tracks[TrackIndex].Keys.IsValidIndex(KeyIndex))
	{
		TouchTrack(TrackIndex);
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

void UAnimStitchTimelineAsset::SetTrackChannelKey(int32 TrackIndex, const FAnimStitchTrackKey& Key, uint8 Channels)
{
	if (!Timeline.Tracks.IsValidIndex(TrackIndex)) { return; }
	FAnimStitchTrack& Track = Timeline.Tracks[TrackIndex];
	if (Track.Kind != EAnimStitchTrackKind::Bone) { SetTrackKey(TrackIndex, Key); return; }
	Channels &= FAnimStitchManualEdits::GetEditableChannels(Track);
	if (Channels == 0) { return; }
	TouchTrack(TrackIndex);
	if (Track.Keys.Num() == 0 && Key.Time > 1e-3 && Track.Mode != EAnimStitchTrackMode::Absolute)
	{
		// Anchor (identity) on the same channels, so the correction does not apply to the whole timeline before the first key.
		FAnimStitchTrackKey Anchor;
		Anchor.Time = 0.0;
		FAnimStitchManualEdits::SetChannelKey(Track, Anchor, Channels);
	}
	FAnimStitchManualEdits::SetChannelKey(Track, Key, Channels);
}

void UAnimStitchTimelineAsset::RemoveTrackChannelKey(int32 TrackIndex, int32 KeyIndex, uint8 Channels)
{
	if (!Timeline.Tracks.IsValidIndex(TrackIndex) || !Timeline.Tracks[TrackIndex].Keys.IsValidIndex(KeyIndex)) { return; }
	FAnimStitchTrack& Track = Timeline.Tracks[TrackIndex];
	if (Track.Kind != EAnimStitchTrackKind::Bone) { RemoveTrackKey(TrackIndex, KeyIndex); return; }
	// Deleting is always allowed: channel locks only guard editing, moving and adding.
	Channels &= Track.Keys[KeyIndex].Channels;
	if (Channels == 0) { return; }
	TouchTrack(TrackIndex);
	Track.Keys[KeyIndex].Channels &= ~Channels;
	if ((Track.Keys[KeyIndex].Channels & AnimStitchChannel::All) == 0) { Track.Keys.RemoveAt(KeyIndex); }
}

int32 UAnimStitchTimelineAsset::MoveTrackChannelKey(int32 TrackIndex, int32 KeyIndex, uint8 Channels, double NewTime)
{
	if (!Timeline.Tracks.IsValidIndex(TrackIndex) || !Timeline.Tracks[TrackIndex].Keys.IsValidIndex(KeyIndex)) { return KeyIndex; }
	FAnimStitchTrack& Track = Timeline.Tracks[TrackIndex];
	if (Track.Kind != EAnimStitchTrackKind::Bone) { return MoveTrackKey(TrackIndex, KeyIndex, NewTime); }
	Channels &= FAnimStitchManualEdits::GetEditableChannels(Track) & Track.Keys[KeyIndex].Channels;
	NewTime = FMath::Max(0.0, NewTime);
	if (Channels == 0 || FMath::IsNearlyEqual(Track.Keys[KeyIndex].Time, NewTime, 1e-9)) { return KeyIndex; }
	TouchTrack(TrackIndex);
	FAnimStitchTrackKey Moved = Track.Keys[KeyIndex];
	Track.Keys[KeyIndex].Channels &= ~Channels;
	if ((Track.Keys[KeyIndex].Channels & AnimStitchChannel::All) == 0) { Track.Keys.RemoveAt(KeyIndex); }
	Moved.Time = NewTime;
	return FAnimStitchManualEdits::SetChannelKey(Track, Moved, Channels);
}

void UAnimStitchTimelineAsset::SetTrackChannelUnlocked(int32 TrackIndex, uint8 Channel, bool bUnlocked)
{
	if (!Timeline.Tracks.IsValidIndex(TrackIndex)) { return; }
	TouchManual();
	uint8& Mask = Timeline.Tracks[TrackIndex].UnlockedChannels;
	Mask = bUnlocked ? (Mask | Channel) : (Mask & ~Channel);
}

void UAnimStitchTimelineAsset::SetTrackControlStyle(int32 TrackIndex, bool bPole, const FAnimStitchControlStyle& Style)
{
	if (Timeline.Tracks.IsValidIndex(TrackIndex))
	{
		TouchManual();
		(bPole ? Timeline.Tracks[TrackIndex].PoleStyle : Timeline.Tracks[TrackIndex].ControlStyle) = Style;
	}
}

void UAnimStitchTimelineAsset::SetTrackShowTrail(int32 TrackIndex, bool bShow)
{
	if (!Timeline.Tracks.IsValidIndex(TrackIndex)) { return; }
	TouchManual();
	Timeline.Tracks[TrackIndex].bShowTrail = bShow;
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
const double SourceTime = Second.SourceTimeAt(TimelineTime);
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
SetTrim(Index, Clip.SourceTimeAt(TimelineTime), Clip.TrimOut);
	return true;
}

bool UAnimStitchTimelineAsset::TrimClipEndTo(int32 Index, double TimelineTime)
{
	if (!CanCutClipAt(Index, TimelineTime))
	{
		return false;
	}
	const FAnimStitchClip& Clip = Timeline.Clips[Index];
SetTrim(Index, Clip.TrimIn, Clip.SourceTimeAt(TimelineTime));
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






