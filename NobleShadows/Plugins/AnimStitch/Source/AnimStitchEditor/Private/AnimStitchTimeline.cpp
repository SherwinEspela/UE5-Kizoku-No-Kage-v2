#include "AnimStitchTimeline.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "ReferenceSkeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

double FAnimStitchTimeline::GetMaxOverlap(int32 Index) const
{
	if (Index <= 0 || !Clips.IsValidIndex(Index))
	{
		return 0.0;
	}
	// Stay strictly inside both clips so ordering and "at most two clips active" always hold.
	constexpr double Epsilon = 1e-3;
	double Max = FMath::Min(Clips[Index - 1].GetDuration(), Clips[Index].GetDuration());
	if (Index >= 2)
	{
		Max = FMath::Min(Max, Clips[Index - 1].GetEndTime() - Clips[Index - 2].GetEndTime());
	}
	return FMath::Max(0.0, Max - Epsilon);
}

void FAnimStitchTimeline::Relayout()
{
	for (int32 i = 0; i < Clips.Num(); ++i)
	{
		FAnimStitchClip& Clip = Clips[i];
		if (i == 0)
		{
			Clip.OverlapIn = 0.0;
			Clip.StartTime = 0.0;
		}
		else
		{
			Clip.OverlapIn = FMath::Clamp(Clip.OverlapIn, 0.0, GetMaxOverlap(i));
			Clip.StartTime = Clips[i - 1].GetEndTime() - Clip.OverlapIn;
		}
	}
	if (!Clips.IsValidIndex(RootAuthorityIndex))
	{
		RootAuthorityIndex = 0;
	}
}

double FAnimStitchTimeline::GetTotalLength() const
{
	double End = 0.0;
	for (const FAnimStitchClip& Clip : Clips)
	{
		End = FMath::Max(End, Clip.GetEndTime());
	}
	return End;
}

bool FAnimStitchEvaluator::Initialize(const FAnimStitchTimeline& InTimeline, FString& OutError)
{
	Timeline = InTimeline;
	Timeline.Relayout();
	const int32 Num = Timeline.Clips.Num();
	if (Num == 0)
	{
		OutError = LOCTEXT("ErrNoClips", "Timeline has no clips.").ToString();
		return false;
	}
	if (!Timeline.Clips.IsValidIndex(Timeline.RootAuthorityIndex))
	{
		Timeline.RootAuthorityIndex = 0;
	}

	Skeleton = nullptr;
	HasTrack.Reset();
	for (int32 i = 0; i < Num; ++i)
	{
		const FAnimStitchClip& Clip = Timeline.Clips[i];
		const UAnimSequence* Seq = Clip.Sequence.Get();
		if (!Seq || !Seq->GetSkeleton() || !Seq->GetDataModel())
		{
			OutError = FText::Format(LOCTEXT("ErrBadClip", "Clip {0} has no valid animation sequence."), i).ToString();
			return false;
		}
		if (!Skeleton)
		{
			Skeleton = Seq->GetSkeleton();
		}
		else if (Skeleton != Seq->GetSkeleton())
		{
			OutError = FText::Format(LOCTEXT("ErrSkelMismatch", "Clip '{0}' uses a different skeleton than the first clip."), FText::FromString(Seq->GetName())).ToString();
			return false;
		}
		if (Clip.GetDuration() <= KINDA_SMALL_NUMBER)
		{
			OutError = FText::Format(LOCTEXT("ErrZeroLen", "Clip {0} has zero length after trimming."), i).ToString();
			return false;
		}
		if (i > 0)
		{
			const FAnimStitchClip& Prev = Timeline.Clips[i - 1];
			if (Clip.StartTime > Prev.GetEndTime() + KINDA_SMALL_NUMBER)
			{
				OutError = FText::Format(LOCTEXT("ErrGap", "Gap between clip {0} and {1}."), i - 1, i).ToString();
				return false;
			}
			if (Clip.StartTime < Prev.StartTime || Clip.GetEndTime() < Prev.GetEndTime())
			{
				OutError = FText::Format(LOCTEXT("ErrOrder", "Clip {0} must start after and end after clip {1}."), i, i - 1).ToString();
				return false;
			}
			if (i > 1 && Clip.StartTime < Timeline.Clips[i - 2].GetEndTime() - KINDA_SMALL_NUMBER)
			{
				OutError = FText::Format(LOCTEXT("ErrTriple", "Clip {0} overlaps more than one neighbour."), i).ToString();
				return false;
			}
		}
	}

	const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
	NumBones = RefSkel.GetNum();

	HasTrack.SetNum(Num);
	for (int32 i = 0; i < Num; ++i)
	{
		const IAnimationDataModel* Model = Timeline.Clips[i].Sequence->GetDataModel();
		HasTrack[i].SetNum(NumBones);
		for (int32 b = 0; b < NumBones; ++b)
		{
			HasTrack[i][b] = Model->IsValidBoneTrackName(RefSkel.GetBoneName(b));
		}
	}

	// Root anchors. Authority is raw; clips after it continue from where the previous clip's
	// effective root is at their start; clips before it end where the next clip's effective root is.
	RootAnchor.Init(FTransform::Identity, Num);
	const int32 Auth = Timeline.RootAuthorityIndex;
	for (int32 i = Auth + 1; i < Num; ++i)
	{
		RootAnchor[i] = EvaluateClipRoot(i - 1, Timeline.Clips[i].StartTime);
	}
	for (int32 i = Auth - 1; i >= 0; --i)
	{
		RootAnchor[i] = EvaluateClipRoot(i + 1, Timeline.Clips[i].GetEndTime());
	}
	return true;
}

FTransform FAnimStitchEvaluator::SampleClipRawRoot(int32 ClipIndex, double SourceTime) const
{
	const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
	if (!HasTrack[ClipIndex][0])
	{
		return RefSkel.GetRefBonePose()[0];
	}
	const IAnimationDataModel* Model = Timeline.Clips[ClipIndex].Sequence->GetDataModel();
	const double Clamped = FMath::Clamp(SourceTime, 0.0, Model->GetPlayLength());
	return Model->EvaluateBoneTrackTransform(RefSkel.GetBoneName(0), Model->GetFrameRate().AsFrameTime(Clamped), EAnimInterpolationType::Linear);
}

FTransform FAnimStitchEvaluator::EvaluateClipRoot(int32 ClipIndex, double TimelineTime) const
{
	const FAnimStitchClip& Clip = Timeline.Clips[ClipIndex];
	const double SrcTime = Clip.TrimIn + (TimelineTime - Clip.StartTime);
	const FTransform Raw = SampleClipRawRoot(ClipIndex, SrcTime);
	const int32 Auth = Timeline.RootAuthorityIndex;
	if (ClipIndex == Auth)
	{
		return Raw;
	}
	// Motion relative to the clip's anchor frame (its first frame for later clips, last for earlier ones),
	// re-expressed in the continuing world frame.
	const double AnchorSrcTime = (ClipIndex > Auth) ? Clip.TrimIn : Clip.TrimOut;
	const FTransform AnchorRaw = SampleClipRawRoot(ClipIndex, AnchorSrcTime);
	return Raw.GetRelativeTransform(AnchorRaw) * RootAnchor[ClipIndex];
}

void FAnimStitchEvaluator::SampleClipRaw(int32 ClipIndex, double TimelineTime, TArray<FTransform>& OutPose) const
{
	const FAnimStitchClip& Clip = Timeline.Clips[ClipIndex];
	const IAnimationDataModel* Model = Clip.Sequence->GetDataModel();
	const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
	const double SrcTime = FMath::Clamp(Clip.TrimIn + (TimelineTime - Clip.StartTime), 0.0, Model->GetPlayLength());
	const FFrameTime FrameTime = Model->GetFrameRate().AsFrameTime(SrcTime);

	OutPose.SetNumUninitialized(NumBones);
	for (int32 b = 0; b < NumBones; ++b)
	{
		OutPose[b] = HasTrack[ClipIndex][b]
			? Model->EvaluateBoneTrackTransform(RefSkel.GetBoneName(b), FrameTime, EAnimInterpolationType::Linear)
			: RefSkel.GetRefBonePose()[b];
	}
	OutPose[0] = EvaluateClipRoot(ClipIndex, TimelineTime);
}

float FAnimStitchEvaluator::ApplyCurve(EAnimStitchBlendCurve Curve, float Alpha)
{
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	return Curve == EAnimStitchBlendCurve::Smoothstep ? Alpha * Alpha * (3.f - 2.f * Alpha) : Alpha;
}

void FAnimStitchEvaluator::EvaluatePose(double T, TArray<FTransform>& OutPose) const
{
	const int32 Num = Timeline.Clips.Num();
	T = FMath::Clamp(T, 0.0, Timeline.GetTotalLength());

	// Find the active clip(s): the last clip that has started, and whether its predecessor still overlaps.
	int32 Cur = 0;
	for (int32 i = 0; i < Num; ++i)
	{
		if (Timeline.Clips[i].StartTime <= T + KINDA_SMALL_NUMBER)
		{
			Cur = i;
		}
	}
	SampleClipRaw(Cur, T, OutPose);

	if (Cur > 0 && T < Timeline.Clips[Cur - 1].GetEndTime())
	{
		const FAnimStitchClip& In = Timeline.Clips[Cur];
		const double OverlapLen = Timeline.Clips[Cur - 1].GetEndTime() - In.StartTime;
		const float Alpha = ApplyCurve(In.BlendIn, OverlapLen > KINDA_SMALL_NUMBER ? float((T - In.StartTime) / OverlapLen) : 1.f);

		TArray<FTransform> Outgoing;
		SampleClipRaw(Cur - 1, T, Outgoing);
		for (int32 b = 0; b < NumBones; ++b)
		{
			FTransform& Dst = OutPose[b]; // incoming
			const FTransform& Src = Outgoing[b];
			const FVector Loc = FMath::Lerp(Src.GetTranslation(), Dst.GetTranslation(), Alpha);
			const FVector Scl = FMath::Lerp(Src.GetScale3D(), Dst.GetScale3D(), Alpha);
			const FQuat Rot = FQuat::Slerp(Src.GetRotation(), Dst.GetRotation(), Alpha).GetNormalized();
			Dst = FTransform(Rot, Loc, Scl);
		}
	}
}

FTransform FAnimStitchEvaluator::EvaluateRoot(double T) const
{
	const int32 Num = Timeline.Clips.Num();
	T = FMath::Clamp(T, 0.0, Timeline.GetTotalLength());
	int32 Cur = 0;
	for (int32 i = 0; i < Num; ++i)
	{
		if (Timeline.Clips[i].StartTime <= T + KINDA_SMALL_NUMBER)
		{
			Cur = i;
		}
	}
	FTransform Root = EvaluateClipRoot(Cur, T);
	if (Cur > 0 && T < Timeline.Clips[Cur - 1].GetEndTime())
	{
		const FAnimStitchClip& In = Timeline.Clips[Cur];
		const double OverlapLen = Timeline.Clips[Cur - 1].GetEndTime() - In.StartTime;
		const float Alpha = ApplyCurve(In.BlendIn, OverlapLen > KINDA_SMALL_NUMBER ? float((T - In.StartTime) / OverlapLen) : 1.f);
		const FTransform Out = EvaluateClipRoot(Cur - 1, T);
		Root = FTransform(FQuat::Slerp(Out.GetRotation(), Root.GetRotation(), Alpha).GetNormalized(),
			FMath::Lerp(Out.GetTranslation(), Root.GetTranslation(), Alpha), FMath::Lerp(Out.GetScale3D(), Root.GetScale3D(), Alpha));
	}
	return Root;
}

float FAnimStitchEvaluator::EvaluateBlendAlpha(int32 IncomingIndex, double TimelineTime) const
{
	if (IncomingIndex <= 0 || !Timeline.Clips.IsValidIndex(IncomingIndex))
	{
		return 0.f;
	}
	const FAnimStitchClip& In = Timeline.Clips[IncomingIndex];
	const double OverlapLen = Timeline.Clips[IncomingIndex - 1].GetEndTime() - In.StartTime;
	if (OverlapLen <= KINDA_SMALL_NUMBER || TimelineTime < In.StartTime)
	{
		return 0.f;
	}
	return ApplyCurve(In.BlendIn, float((TimelineTime - In.StartTime) / OverlapLen));
}

void FAnimStitchEvaluator::EvaluateClipPose(int32 ClipIndex, double TimelineTime, TArray<FTransform>& OutLocalPose) const
{
	SampleClipRaw(ClipIndex, TimelineTime, OutLocalPose);
}

#undef LOCTEXT_NAMESPACE

