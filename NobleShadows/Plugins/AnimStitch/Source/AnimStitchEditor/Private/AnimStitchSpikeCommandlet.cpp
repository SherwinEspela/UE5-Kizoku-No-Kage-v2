#include "AnimStitchSpikeCommandlet.h"
#include "AnimStitchBaker.h"
#include "AnimStitchTimelineAsset.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "ReferenceSkeleton.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimStitchSpike, Log, All);

int32 UAnimStitchSpikeCommandlet::Main(const FString& Params)
{
	FString PathA, PathB, Out;
	double Overlap = 0.3;
	int32 Authority = 0;
	FParse::Value(*Params, TEXT("A="), PathA);
	FParse::Value(*Params, TEXT("B="), PathB);
	FParse::Value(*Params, TEXT("Out="), Out);
	FParse::Value(*Params, TEXT("Overlap="), Overlap);
	FParse::Value(*Params, TEXT("Authority="), Authority);

	const UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *PathA);
	const UAnimSequence* B = LoadObject<UAnimSequence>(nullptr, *PathB);
	if (!A || !B)
	{
		UE_LOG(LogAnimStitchSpike, Error, TEXT("Could not load A/B"));
		return 1;
	}

	// Phase 2 model test: -C=<other-skeleton sequence> exercises mismatch, move, overlap clamps and evaluator validity.
	FString PathC;
	if (FParse::Value(*Params, TEXT("C="), PathC))
	{
		UAnimStitchTimelineAsset* Asset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> Warn;
		const UAnimSequence* C = LoadObject<UAnimSequence>(nullptr, *PathC);
		const int32 Added = Asset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B), const_cast<UAnimSequence*>(C) }, INDEX_NONE, Warn);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("ModelTest: added %d of 3, warnings %d: %s"), Added, Warn.Num(), Warn.Num() ? *Warn[0].ToString() : TEXT(""));
		Asset->SetOverlap(1, 999.0);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("ModelTest: overlap clamp 999 -> %.3f (max %.3f)"), Asset->Timeline.Clips[1].OverlapIn, Asset->Timeline.GetMaxOverlap(1));
		Asset->SetTrim(0, 1.0, 2.0);
		Asset->DuplicateClip(0);
		Asset->MoveClip(2, 0);
		Asset->SetOverlap(1, 0.5);
		Asset->SetTrim(1, 0.0, 0.1); // short clip must keep overlap legal
		FAnimStitchEvaluator Eval;
		FString EvalError;
		const bool bOk = Eval.Initialize(Asset->Timeline, EvalError);
		for (int32 i = 0; i < Asset->Timeline.Clips.Num(); ++i)
		{
			const FAnimStitchClip& K = Asset->Timeline.Clips[i];
			UE_LOG(LogAnimStitchSpike, Display, TEXT("  clip %d %s start %.3f dur %.3f overlap %.3f"), i, *GetNameSafe(K.Sequence), K.StartTime, K.GetDuration(), K.OverlapIn);
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("ModelTest: evaluator init %s %s"), bOk ? TEXT("OK") : TEXT("FAILED"), *EvalError);
		return bOk ? 0 : 3;
	}

	FAnimStitchTimeline TL;
	FAnimStitchClip CA, CB;
	CA.Sequence = const_cast<UAnimSequence*>(A); CA.TrimIn = 0; CA.TrimOut = A->GetPlayLength();
	CB.Sequence = const_cast<UAnimSequence*>(B); CB.TrimIn = 0; CB.TrimOut = B->GetPlayLength(); CB.OverlapIn = Overlap;
	TL.Clips = { CA, CB };
	TL.RootAuthorityIndex = Authority;
	TL.Relayout();
	CB = TL.Clips[1];

	int32 Fps = 0;
	FParse::Value(*Params, TEXT("Fps="), Fps);
	FString Error;
	UAnimSequence* Baked = FAnimStitchBaker::Bake(TL, Out, Fps > 0 ? FFrameRate(Fps, 1) : FFrameRate(), true, Error);
	if (!Baked)
	{
		UE_LOG(LogAnimStitchSpike, Error, TEXT("Bake failed: %s"), *Error);
		return 2;
	}

	const IAnimationDataModel* M = Baked->GetDataModel();
	const FName RootName = Baked->GetSkeleton()->GetReferenceSkeleton().GetBoneName(0);
	const double JoinT = CB.StartTime;
	UE_LOG(LogAnimStitchSpike, Display, TEXT("Baked: %d frames, %.3fs, rate %.1f, tracks %d, rootmotion=%d. A=%.3fs B=%.3fs join@%.3f authority=%d"),
		M->GetNumberOfFrames(), Baked->GetPlayLength(), M->GetFrameRate().AsDecimal(), M->GetNumBoneTracks(), Baked->bEnableRootMotion, A->GetPlayLength(), B->GetPlayLength(), JoinT, Authority);

	// Report root translation/yaw around the join and the largest per-frame root step overall.
	const double Rate = M->GetFrameRate().AsDecimal();
	double MaxStep = 0.0; int32 MaxStepFrame = 0;
	FVector Prev = FVector::ZeroVector;
	for (int32 f = 0; f <= M->GetNumberOfFrames(); ++f)
	{
		const FTransform T = M->GetBoneTrackTransform(RootName, FFrameNumber(f));
		if (f > 0 && (T.GetTranslation() - Prev).Size() > MaxStep)
		{
			MaxStep = (T.GetTranslation() - Prev).Size(); MaxStepFrame = f;
		}
		Prev = T.GetTranslation();
		const double t = f / Rate;
		if (FMath::Abs(t - JoinT) < 0.5 / Rate || FMath::Abs(t - (JoinT + Overlap)) < 0.5 / Rate || f == 0 || f == M->GetNumberOfFrames())
		{
			UE_LOG(LogAnimStitchSpike, Display, TEXT("  f%d t=%.3f root loc=%s yaw=%.1f"), f, t, *T.GetTranslation().ToCompactString(), T.Rotator().Yaw);
		}
	}
	UE_LOG(LogAnimStitchSpike, Display, TEXT("Max root step %.3f at frame %d"), MaxStep, MaxStepFrame);
	return 0;
}
