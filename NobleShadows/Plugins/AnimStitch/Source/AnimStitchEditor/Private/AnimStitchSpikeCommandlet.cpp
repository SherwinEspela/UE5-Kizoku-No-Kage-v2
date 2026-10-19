#include "AnimStitchSpikeCommandlet.h"
#include "AnimStitchBaker.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchBatch.h"
#include "AnimStitchBlueprintLibrary.h"
#include "AnimStitchFootLock.h"
#include "Algo/MaxElement.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
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

	// Phase 7 test: -PredictTest=/Game/Folder with -A=<run stop> -B=<idle>. Bakes both-off baseline vs Predict Feet ON (foot lock OFF).
	FString PredictFolder;
	if (FParse::Value(*Params, TEXT("PredictTest="), PredictFolder))
	{
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		if (TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W) != 2)
		{
			UE_LOG(LogAnimStitchSpike, Error, TEXT("PredictTest: could not add both clips: %s"), W.Num() ? *W[0].ToString() : TEXT("?"));
			return 20;
		}
		double Overlap3 = 0.4;
		FParse::Value(*Params, TEXT("Overlap="), Overlap3);
		TLAsset->SetTrim(1, 0.0, FMath::Min(2.5, (double)B->GetPlayLength()));
		TLAsset->SetOverlap(1, Overlap3);
		TLAsset->SetRootAuthority(0);

		FString E1, E2;
		UAnimSequence* Base = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, PredictFolder / TEXT("AS_PredBase"), 30, false, E1);
		TLAsset->Timeline.bPredictFeet = true;
		UAnimSequence* Pred = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, PredictFolder / TEXT("AS_PredOn"), 30, false, E2);
		if (!Base || !Pred) { UE_LOG(LogAnimStitchSpike, Error, TEXT("PredictTest bake failed: base='%s' pred='%s'"), *E1, *E2); return 21; }

		TLAsset->Timeline.bPredictFeet = false;
		FAnimStitchEvaluator Eval;
		FString EvalErr;
		Eval.Initialize(TLAsset->Timeline, EvalErr);
		const FReferenceSkeleton& Ref = Eval.GetSkeleton()->GetReferenceSkeleton();
		auto ReadPose = [&Ref](const UAnimSequence* Seq, int32 Frame, TArray<FTransform>& OutPose)
		{
			const IAnimationDataModel* Model = Seq->GetDataModel();
			OutPose = Ref.GetRefBonePose();
			TArray<FName> Names;
			Model->GetBoneTrackNames(Names);
			for (const FName& N : Names)
			{
				const int32 b = Ref.FindBoneIndex(N);
				if (b != INDEX_NONE) { OutPose[b] = Model->GetBoneTrackTransform(N, FFrameNumber(Frame)); }
			}
		};
		auto CS = [&Ref](const TArray<FTransform>& Local, int32 Bone)
		{
			FTransform T = Local[Bone];
			for (int32 P = Ref.GetParentIndex(Bone); P != INDEX_NONE; P = Ref.GetParentIndex(P)) { T = T * Local[P]; }
			return T;
		};

		const int32 NumFrames = Base->GetDataModel()->GetNumberOfFrames();
		TArray<TArray<FTransform>> BasePoses, PredPoses, APoses, BPoses;
		BasePoses.SetNum(NumFrames + 1); PredPoses.SetNum(NumFrames + 1); APoses.SetNum(NumFrames + 1); BPoses.SetNum(NumFrames + 1);
		const FAnimStitchTimeline& TL = TLAsset->Timeline;
		for (int32 f = 0; f <= NumFrames; ++f)
		{
			ReadPose(Base, f, BasePoses[f]); ReadPose(Pred, f, PredPoses[f]);
			Eval.EvaluateClipPose(0, f / 30.0, APoses[f]);
			Eval.EvaluateClipPose(1, f / 30.0, BPoses[f]);
		}
		const int32 WinStart = FMath::CeilToInt32(TL.Clips[1].StartTime * 30.0), WinEnd = FMath::FloorToInt32(TL.Clips[0].GetEndTime() * 30.0);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("PredictTest: %d frames, crossfade frames %d..%d (%.2fs), skeleton bones %d"), NumFrames, WinStart, WinEnd, TL.Clips[1].OverlapIn, Ref.GetNum());

		FName L, R;
		FAnimStitchFootLock::ResolveFootBones(Eval.GetSkeleton(), TL.FootLock, L, R);
		const FAnimStitchFootChain Chains[2] = { FAnimStitchFootLock::BuildChain(Ref, L), FAnimStitchFootLock::BuildChain(Ref, R) };

		// What the pass reports on the same blended frames the bake used.
		{
			TArray<TArray<FTransform>> Raw;
			Raw.SetNum(NumFrames + 1);
			for (int32 f = 0; f <= NumFrames; ++f) { Eval.EvaluatePose(f / 30.0, Raw[f]); }
			FAnimStitchTimeline WithPredict = TL;
			WithPredict.bPredictFeet = true;
			FAnimStitchEvaluator EvalPredict;
			EvalPredict.Initialize(WithPredict, EvalErr);
			const FAnimStitchPredictReport Report = FAnimStitchFootLock::PredictFeet(EvalPredict, Raw, 30.0);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("PredictTest report: valid=%d, crossfades %d, %d foot-frames moved, %d frames faded for reach | fully weighted foot vs target (XY): before %.3f cm -> after %.3f cm"),
				Report.bValid, Report.Overlaps, Report.PinnedFootFrames, Report.ReachReleasedFrames, Report.MaxDeviationBefore, Report.MaxDeviationAfter);
		}

		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			const FAnimStitchFootChain& C = Chains[Foot];
			double PathBase = 0.0, PathPred = 0.0, MaxStepBase = 0.0, MaxStepPred = 0.0, MaxLegErr = 0.0, MaxPelvis = 0.0, MaxOutside = 0.0;
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				const FVector Pb = CS(BasePoses[f], C.Foot).GetTranslation(), Pp = CS(PredPoses[f], C.Foot).GetTranslation();
				MaxPelvis = FMath::Max(MaxPelvis, (CS(BasePoses[f], C.ThighParent).GetTranslation() - CS(PredPoses[f], C.ThighParent).GetTranslation()).Size());
				const FVector Hb = CS(BasePoses[f], C.Thigh).GetTranslation(), Kb = CS(BasePoses[f], C.Calf).GetTranslation();
				const FVector Hp = CS(PredPoses[f], C.Thigh).GetTranslation(), Kp = CS(PredPoses[f], C.Calf).GetTranslation();
				MaxLegErr = FMath::Max(MaxLegErr, FMath::Abs((Kp - Hp).Size() - (Kb - Hb).Size()));
				if (f > 0)
				{
					const double Sb = FVector2D(Pb - CS(BasePoses[f - 1], C.Foot).GetTranslation()).Size();
					const double Sp = FVector2D(Pp - CS(PredPoses[f - 1], C.Foot).GetTranslation()).Size();
					if (f >= WinStart && f <= WinEnd) { PathBase += Sb; PathPred += Sp; MaxStepBase = FMath::Max(MaxStepBase, Sb); MaxStepPred = FMath::Max(MaxStepPred, Sp); }
				}
				if (f < WinStart - 1 || f > WinEnd + 26) { MaxOutside = FMath::Max(MaxOutside, (Pb - Pp).Size()); }
			}
			const FVector EndBase = CS(BasePoses[WinEnd], C.Foot).GetTranslation(), EndPred = CS(PredPoses[WinEnd], C.Foot).GetTranslation(), EndB = CS(BPoses[WinEnd], C.Foot).GetTranslation();
			UE_LOG(LogAnimStitchSpike, Display, TEXT("PredictTest foot %d (%s): crossfade foot path (XY) baseline %.2f cm -> predicted %.2f cm | biggest per-frame step %.2f -> %.2f cm | at crossfade end distance to the idle's own foot spot: baseline %.2f cm -> predicted %.2f cm | pelvis moved %.4f cm | leg length change %.5f cm | changed outside the crossfade (+0.8 s): %.4f cm"),
				Foot, *Ref.GetBoneName(C.Foot).ToString(), PathBase, PathPred, MaxStepBase, MaxStepPred,
				FVector2D(EndBase - EndB).Size(), FVector2D(EndPred - EndB).Size(), MaxPelvis, MaxLegErr, MaxOutside);
		}
		return 0;
	}

	// Phase 6 test: -FootTest=/Game/Folder with -A=<landing> -B=<run>. Bakes foot lock OFF and ON and measures plant slide.
	FString FootFolder;
	if (FParse::Value(*Params, TEXT("FootTest="), FootFolder))
	{
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		if (TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W) != 2)
		{
			UE_LOG(LogAnimStitchSpike, Error, TEXT("FootTest: could not add both clips: %s"), W.Num() ? *W[0].ToString() : TEXT("?"));
			return 10;
		}
		double Overlap2 = 0.2;
		FParse::Value(*Params, TEXT("Overlap="), Overlap2);
		TLAsset->SetTrim(1, 0.0, FMath::Min(3.0, (double)B->GetPlayLength()));
		TLAsset->SetOverlap(1, Overlap2);
		TLAsset->SetRootAuthority(0);

		FString E1, E2;
		UAnimSequence* Off = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, FootFolder / TEXT("AS_FootOff"), 30, false, E1);
		FAnimStitchFootLockSettings FL;
		FL.bEnabled = true;
		TLAsset->Timeline.FootLock = FL;
		UAnimSequence* On = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, FootFolder / TEXT("AS_FootOn"), 30, false, E2);
		if (!Off || !On)
		{
			UE_LOG(LogAnimStitchSpike, Error, TEXT("FootTest bake failed: off='%s' on='%s'"), *E1, *E2);
			return 11;
		}
		TLAsset->Timeline.FootLock.bEnabled = false;
		FAnimStitchEvaluator Eval;
		FString EvalErr;
		Eval.Initialize(TLAsset->Timeline, EvalErr);
		const FReferenceSkeleton& Ref = Eval.GetSkeleton()->GetReferenceSkeleton();

		auto ReadPose = [&Ref](const UAnimSequence* Seq, int32 Frame, TArray<FTransform>& OutPose)
		{
			const IAnimationDataModel* Model = Seq->GetDataModel();
			OutPose = Ref.GetRefBonePose();
			TArray<FName> Names;
			Model->GetBoneTrackNames(Names);
			for (const FName& N : Names)
			{
				const int32 b = Ref.FindBoneIndex(N);
				if (b != INDEX_NONE) { OutPose[b] = Model->GetBoneTrackTransform(N, FFrameNumber(Frame)); }
			}
		};
		auto CS = [&Ref](const TArray<FTransform>& Local, int32 Bone)
		{
			FTransform T = Local[Bone];
			for (int32 P = Ref.GetParentIndex(Bone); P != INDEX_NONE; P = Ref.GetParentIndex(P)) { T = T * Local[P]; }
			return T;
		};

		const int32 NumFrames = Off->GetDataModel()->GetNumberOfFrames();
		TArray<TArray<FTransform>> OffPoses, OnPoses, RefPoses;
		OffPoses.SetNum(NumFrames + 1); OnPoses.SetNum(NumFrames + 1); RefPoses.SetNum(NumFrames + 1);
		double MaxOffVsEval = 0.0;
		for (int32 f = 0; f <= NumFrames; ++f)
		{
			ReadPose(Off, f, OffPoses[f]);
			ReadPose(On, f, OnPoses[f]);
			Eval.EvaluatePose(f / 30.0, RefPoses[f]);
			for (int32 b = 0; b < Ref.GetNum(); ++b)
			{
				MaxOffVsEval = FMath::Max(MaxOffVsEval, (OffPoses[f][b].GetTranslation() - RefPoses[f][b].GetTranslation()).Size());
				MaxOffVsEval = FMath::Max(MaxOffVsEval, FMath::Abs(1.0 - FMath::Abs(OffPoses[f][b].GetRotation() | RefPoses[f][b].GetRotation())) * 1000.0);
			}
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest: %d frames, %.2fs, skeleton bones %d"), NumFrames, Off->GetPlayLength(), Ref.GetNum());
		UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest: foot lock OFF vs plain evaluator, max deviation = %.6f (0 = identical)"), MaxOffVsEval);

		TLAsset->Timeline.FootLock = FL;
		FAnimStitchEvaluator EvalOn;
		EvalOn.Initialize(TLAsset->Timeline, EvalErr);
		const FAnimStitchFootLockResult Det = FAnimStitchFootLock::Detect(EvalOn, OffPoses, 30.0);
		if (!Det.bValid)
		{
			UE_LOG(LogAnimStitchSpike, Error, TEXT("FootTest detect: %s"), *Det.Problem.ToString());
			return 12;
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest: feet '%s' / '%s', ground z %.2f"), *Det.FootBones[0].ToString(), *Det.FootBones[1].ToString(), Det.GroundZ);
		const int32 Blend = FMath::Max(1, FMath::RoundToInt(FL.BlendTime * 30.0));
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			const FAnimStitchFootChain& C = Det.Chains[Foot];
			int32 Runs = 0, PlantedFrames = 0;
			double WorstOff = 0.0, WorstOn = 0.0, MaxLegErr = 0.0, MaxPop = 0.0;
			const TArray<uint8>& Mask = Det.Mask[Foot];
			for (int32 f = 0; f <= NumFrames; ++f) { PlantedFrames += AnimStitchPlant::IsPlanted(Mask[f]) ? 1 : 0; }
			for (int32 s = 0; s <= NumFrames;)
			{
				if (!AnimStitchPlant::IsPlanted(Mask[s])) { ++s; continue; }
				int32 e = s;
				while (e + 1 <= NumFrames && AnimStitchPlant::IsPlanted(Mask[e + 1])) { ++e; }
				++Runs;
				FVector MeanOff = FVector::ZeroVector, MeanOn = FVector::ZeroVector;
				const int32 Lo = s + Blend, Hi = e - Blend;
				if (Hi >= Lo)
				{
					for (int32 f = Lo; f <= Hi; ++f) { MeanOff += CS(OffPoses[f], C.Foot).GetTranslation(); MeanOn += CS(OnPoses[f], C.Foot).GetTranslation(); }
					MeanOff /= (Hi - Lo + 1); MeanOn /= (Hi - Lo + 1);
					for (int32 f = Lo; f <= Hi; ++f)
					{
						const FVector PO = CS(OffPoses[f], C.Foot).GetTranslation(), PN = CS(OnPoses[f], C.Foot).GetTranslation();
						WorstOff = FMath::Max(WorstOff, FVector2D(PO.X - MeanOff.X, PO.Y - MeanOff.Y).Size());
						WorstOn = FMath::Max(WorstOn, FVector2D(PN.X - MeanOn.X, PN.Y - MeanOn.Y).Size());
					}
				}
				s = e + 1;
			}
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				const FVector H0 = CS(OffPoses[f], C.Thigh).GetTranslation(), K0 = CS(OffPoses[f], C.Calf).GetTranslation(), A0 = CS(OffPoses[f], C.Foot).GetTranslation();
				const FVector H1 = CS(OnPoses[f], C.Thigh).GetTranslation(), K1 = CS(OnPoses[f], C.Calf).GetTranslation(), A1 = CS(OnPoses[f], C.Foot).GetTranslation();
				MaxLegErr = FMath::Max(MaxLegErr, FMath::Abs((K1 - H1).Size() - (K0 - H0).Size()));
				MaxLegErr = FMath::Max(MaxLegErr, FMath::Abs((A1 - K1).Size() - (A0 - K0).Size()));
				if (f > 0)
				{
					const FVector Prev = CS(OnPoses[f - 1], C.Foot).GetTranslation(), PrevOff = CS(OffPoses[f - 1], C.Foot).GetTranslation();
					MaxPop = FMath::Max(MaxPop, FMath::Abs((A1 - Prev).Size() - (A0 - PrevOff).Size()));
				}
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest foot %d (%s): %d planted frames in %d runs | in-plant XY slide: OFF %.3f cm -> ON %.3f cm | leg length change %.5f cm | extra per-frame foot jump %.3f cm"),
				Foot, *Det.FootBones[Foot].ToString(), PlantedFrames, Runs, WorstOff, WorstOn, MaxLegErr, MaxPop);
		}

		// ---- Persistent manual lock model: Lock at L0 (never auto-released), explicit Unlock at U0 in a second bake. ----
		{
			const int32 Foot = 0;
			const FAnimStitchFootChain& C = Det.Chains[Foot];
			int32 FirstPlanted = 0;
			while (FirstPlanted < NumFrames && !AnimStitchPlant::IsPlanted(Det.Mask[Foot][FirstPlanted])) { ++FirstPlanted; }
			double LockT = (FirstPlanted + 5) / 30.0, UnlockT = LockT + 0.8; // lock a few frames into the first real plant
			FParse::Value(*Params, TEXT("LockT="), LockT);
			FParse::Value(*Params, TEXT("UnlockT="), UnlockT);
			const int32 F0 = FMath::RoundToInt(LockT * 30.0), F1 = FMath::RoundToInt(UnlockT * 30.0);
			auto BakeVariant = [&](const TCHAR* Name, TArray<TArray<FTransform>>& OutPoses) -> bool
			{
				FString Err;
				UAnimSequence* Seq = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, FootFolder / Name, 30, false, Err);
				if (!Seq) { UE_LOG(LogAnimStitchSpike, Error, TEXT("variant %s failed: %s"), Name, *Err); return false; }
				OutPoses.SetNum(NumFrames + 1);
				for (int32 f = 0; f <= NumFrames; ++f) { ReadPose(Seq, f, OutPoses[f]); }
				return true;
			};
			auto Reach = [&](const TArray<FTransform>& Pose)
			{
				return (CS(Pose, C.Calf).GetTranslation() - CS(Pose, C.Thigh).GetTranslation()).Size() + (CS(Pose, C.Foot).GetTranslation() - CS(Pose, C.Calf).GetTranslation()).Size();
			};

			TLAsset->AddFootEvent(Foot, LockT, EAnimStitchFootAction::Lock);
			TArray<TArray<FTransform>> LockOnly;
			if (!BakeVariant(TEXT("AS_FootLockForever"), LockOnly)) { return 13; }
			const FVector Target = CS(OffPoses[F0], C.Foot).GetTranslation();
			int32 Reachable = 0, Unreachable = 0;
			double MaxDev = 0.0;
			for (int32 f = F0; f <= NumFrames; ++f)
			{
				const double Dist = (Target - CS(OffPoses[f], C.Thigh).GetTranslation()).Size();
				if (Dist < Reach(OffPoses[f]) - 0.05)
				{
					++Reachable;
					MaxDev = FMath::Max(MaxDev, (CS(LockOnly[f], C.Foot).GetTranslation() - Target).Size());
				}
				else { ++Unreachable; }
			}
			// Before the first manual event the automatic result must be untouched.
			double MaxPre = 0.0;
			for (int32 f = 0; f < F0; ++f)
			{
				MaxPre = FMath::Max(MaxPre, (CS(LockOnly[f], C.Foot).GetTranslation() - CS(OnPoses[f], C.Foot).GetTranslation()).Size());
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest manual LOCK at %.2fs, never unlocked: frames %d..%d | glued within reach: %d frames, max deviation %.4f cm | out of reach (leg fully extended): %d frames | auto result before the lock unchanged (max diff %.4f cm)"),
				LockT, F0, NumFrames, Reachable, MaxDev, Unreachable, MaxPre);

			// Leg extension over a held lock (Lock..Unlock region), pelvis following ON (default) vs OFF.
			{
				// Hold the lock a few frames past the point where the leg runs out of reach, so the pelvis has something to do.
				int32 FirstOut = F0;
				while (FirstOut < NumFrames && (Target - CS(OffPoses[FirstOut], C.Thigh).GetTranslation()).Size() < Reach(OffPoses[FirstOut]) - 0.05) { ++FirstOut; }
				const int32 PelvisEnd = FMath::Min(NumFrames, FirstOut + 4);
				const float SavedCap = TLAsset->Timeline.FootLock.MaxPelvisOffset;
				TLAsset->Timeline.FootLock.MaxPelvisOffset = 60.f; // generous cap for the demonstration; default is 20
				TLAsset->AddFootEvent(Foot, PelvisEnd / 30.0, EAnimStitchFootAction::Unlock);
				TArray<TArray<FTransform>> WithPelvis, NoPelvis;
				if (!BakeVariant(TEXT("AS_FootLockPelvisOn"), WithPelvis)) { return 15; }
				TLAsset->Timeline.FootLock.bPelvisFollows = false;
				const bool bOk = BakeVariant(TEXT("AS_FootLockPelvisOff"), NoPelvis);
				TLAsset->Timeline.FootLock.bPelvisFollows = true;
				TLAsset->Timeline.FootLock.MaxPelvisOffset = SavedCap;
				if (!bOk) { return 15; }
				const int32 F1p = PelvisEnd;
				auto Extension = [&](const TArray<FTransform>& Pose)
				{
					const FVector H = CS(Pose, C.Thigh).GetTranslation(), K = CS(Pose, C.Calf).GetTranslation(), A = CS(Pose, C.Foot).GetTranslation();
					return (A - H).Size() / ((K - H).Size() + (A - K).Size());
				};
				int32 StraightOn = 0, StraightOff = 0, StraightOrig = 0, Frames = 0;
				double MaxExtOn = 0.0, MaxExtOff = 0.0, MaxExtOrig = 0.0, MaxPelvis = 0.0, MaxDevOn = 0.0, MaxDevOff = 0.0, MaxPelvisElsewhere = 0.0;
				const int32 PelvisBone = C.ThighParent;
				for (int32 f = 0; f <= NumFrames; ++f)
				{
					const double Diff = (CS(WithPelvis[f], PelvisBone).GetTranslation() - CS(NoPelvis[f], PelvisBone).GetTranslation()).Size();
					if (f >= F0 && f < F1p)
					{
						++Frames;
						const double ExtOn = Extension(WithPelvis[f]), ExtOff = Extension(NoPelvis[f]), ExtOrig = Extension(OffPoses[f]);
						StraightOn += ExtOn > 0.995; StraightOff += ExtOff > 0.995; StraightOrig += ExtOrig > 0.995;
						MaxExtOn = FMath::Max(MaxExtOn, ExtOn); MaxExtOff = FMath::Max(MaxExtOff, ExtOff); MaxExtOrig = FMath::Max(MaxExtOrig, ExtOrig);
						MaxPelvis = FMath::Max(MaxPelvis, Diff);
						MaxDevOn = FMath::Max(MaxDevOn, (CS(WithPelvis[f], C.Foot).GetTranslation() - Target).Size());
						MaxDevOff = FMath::Max(MaxDevOff, (CS(NoPelvis[f], C.Foot).GetTranslation() - Target).Size());
					}
					else { MaxPelvisElsewhere = FMath::Max(MaxPelvisElsewhere, Diff); }
				}
				UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest PELVIS over the %d locked frames: legs fully straight (ext>0.995): animation %d (max %.3f) | lock, pelvis OFF %d (max %.3f) -> pelvis FOLLOWS %d (max %.3f) | foot drift from target OFF %.2f -> FOLLOWS %.2f cm | pelvis moved up to %.2f cm inside the lock (cap %.1f), %.2f cm elsewhere"),
					Frames, StraightOrig, MaxExtOrig, StraightOff, MaxExtOff, StraightOn, MaxExtOn, MaxDevOff, MaxDevOn, MaxPelvis, 60.0, MaxPelvisElsewhere);
			}
			TLAsset->AddFootEvent(Foot, UnlockT, EAnimStitchFootAction::Unlock);
			TArray<TArray<FTransform>> Region;
			if (!BakeVariant(TEXT("AS_FootLockRegion"), Region)) { return 14; }
			double MaxDevRegion = 0.0, MaxAfter = 0.0;
			int32 RegionReachable = 0;
			for (int32 f = F0; f < F1; ++f)
			{
				if ((Target - CS(OffPoses[f], C.Thigh).GetTranslation()).Size() < Reach(OffPoses[f]) - 0.05)
				{
					++RegionReachable;
					MaxDevRegion = FMath::Max(MaxDevRegion, (CS(Region[f], C.Foot).GetTranslation() - Target).Size());
				}
			}
			const int32 Settled = F1 + Blend + 2;
			for (int32 f = Settled; f <= NumFrames; ++f)
			{
				MaxAfter = FMath::Max(MaxAfter, (CS(Region[f], C.Foot).GetTranslation() - CS(OffPoses[f], C.Foot).GetTranslation()).Size());
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("FootTest manual REGION %d..%d: glued %d frames, max deviation %.4f cm | after the explicit unlock (+blend) foot follows the animation again from frame %d, max diff to uncorrected %.4f cm (auto-detection stays off after an unlock)"),
				F0, F1 - 1, RegionReachable, MaxDevRegion, Settled, MaxAfter);
		}
		return 0;
	}

	// Bake-time profile of an existing timeline asset: -BakeTL=<asset> -Fps=<n> -Out=<package path>
	FString BakeTL;
	if (FParse::Value(*Params, TEXT("BakeTL="), BakeTL))
	{
		UAnimStitchTimelineAsset* Source = LoadObject<UAnimStitchTimelineAsset>(nullptr, *BakeTL);
		if (!Source) { UE_LOG(LogAnimStitchSpike, Error, TEXT("BakeTL load failed")); return 8; }
		int32 ProfileFps = 0;
		FParse::Value(*Params, TEXT("Fps="), ProfileFps);
		const double T0 = FPlatformTime::Seconds();
		FString ProfileError;
		UAnimSequence* Result = UAnimStitchBlueprintLibrary::BakeTimeline(Source, Out, ProfileFps, true, ProfileError);
		const double T1 = FPlatformTime::Seconds();
		if (!Result) { UE_LOG(LogAnimStitchSpike, Error, TEXT("BakeTL failed: %s"), *ProfileError); return 9; }
		const IAnimationDataModel* PM = Result->GetDataModel();
		UE_LOG(LogAnimStitchSpike, Display, TEXT("BakeTL: %.2fs total | %d frames @ %.1f fps, %.2fs long, %d tracks (of %d skeleton bones)"), T1 - T0, PM->GetNumberOfFrames(), PM->GetFrameRate().AsDecimal(), Result->GetPlayLength(), PM->GetNumBoneTracks(), Result->GetSkeleton()->GetReferenceSkeleton().GetNum());
		return 0;
	}

	// Phase 5 acceptance: -ThreeClip=/Game/Folder -D=<third clip>. 3 clips, trims, crossfades, authority = middle clip.
	FString ThreeFolder, ThreeD;
	if (FParse::Value(*Params, TEXT("ThreeClip="), ThreeFolder) && FParse::Value(*Params, TEXT("D="), ThreeD))
	{
		UAnimSequence* D = LoadObject<UAnimSequence>(nullptr, *ThreeD);
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B), D }, INDEX_NONE, W);
		TLAsset->SetTrim(0, 0.0, 3.0);
		TLAsset->SetTrim(1, 0.5, 3.5);
		TLAsset->SetOverlap(1, 0.4);
		TLAsset->SetTrim(2, 0.0, 2.0);
		TLAsset->SetOverlap(2, 0.3);
		TLAsset->Timeline.Clips[2].BlendIn = EAnimStitchBlendCurve::Linear;
		TLAsset->SetRootAuthority(1);
		const double ClipBLen = FMath::Min(3.5, (double)B->GetPlayLength()) - 0.5; // trims are clamped to the source length
		const double Expected = 3.0 + ClipBLen - 0.4 + 2.0 - 0.3;
		const FAnimStitchTimeline& TLC = TLAsset->Timeline;

		FString BakeError;
		UAnimSequence* Baked = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, ThreeFolder / TEXT("AS_ThreeClip"), 30, false, BakeError);
		if (!Baked)
		{
			UE_LOG(LogAnimStitchSpike, Error, TEXT("ThreeClip bake failed: %s"), *BakeError);
			return 7;
		}
		const IAnimationDataModel* M = Baked->GetDataModel();
		UE_LOG(LogAnimStitchSpike, Display, TEXT("ThreeClip: %d frames, %.3fs (expected %.3f), %d tracks, rootmotion=%d"), M->GetNumberOfFrames(), Baked->GetPlayLength(), Expected, M->GetNumBoneTracks(), Baked->bEnableRootMotion);

		const FName RootName = Baked->GetSkeleton()->GetReferenceSkeleton().GetBoneName(0);
		TArray<double> Steps;
		FVector Prev = FVector::ZeroVector;
		for (int32 f = 0; f <= M->GetNumberOfFrames(); ++f)
		{
			const FVector P = M->GetBoneTrackTransform(RootName, FFrameNumber(f)).GetTranslation();
			if (f > 0) { Steps.Add((P - Prev).Size()); }
			Prev = P;
		}
		TArray<double> Sorted = Steps;
		Sorted.Sort();
		const double Median = Sorted[Sorted.Num() / 2];
		// Joins: clip1 starts at 2.6 (ends 3.0); clip2 starts at 5.3 (clip1 ends 5.6) — overlaps in output time.
		auto MaxStepIn = [&](double T0, double T1)
		{
			double Max = 0.0;
			for (int32 f = FMath::Max(0, int32(T0 * 30)); f < Steps.Num() && f <= int32(T1 * 30); ++f) { Max = FMath::Max(Max, Steps[f]); }
			return Max;
		};
		UE_LOG(LogAnimStitchSpike, Display, TEXT("ThreeClip root step: median %.3f, global max %.3f, join1 window max %.3f, join2 window max %.3f"), Median, *Algo::MaxElement(Steps), MaxStepIn(TLC.Clips[1].StartTime - 0.1, TLC.Clips[0].GetEndTime() + 0.1), MaxStepIn(TLC.Clips[2].StartTime - 0.1, TLC.Clips[1].GetEndTime() + 0.1));

		// Overwrite rules: existing asset is refused without the flag, replaced with it.
		FString Err2, Err3;
		const bool bRefused = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, ThreeFolder / TEXT("AS_ThreeClip"), 30, false, Err2) == nullptr;
		UAnimSequence* Replaced = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, ThreeFolder / TEXT("AS_ThreeClip"), 30, true, Err3);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("ThreeClip overwrite: refused without flag=%d, replaced with flag=%d (%s)"), bRefused, Replaced != nullptr, *Err3);
		return 0;
	}

	// Phase 4 test: -Batch=/Game/Folder -D=<another clip>. Saves a template asset, then bakes A + {B, D}.
	FString BatchFolder, PathD, LoadTL;
	if (FParse::Value(*Params, TEXT("LoadTL="), LoadTL))
	{
		const UAnimStitchTimelineAsset* Loaded = LoadObject<UAnimStitchTimelineAsset>(nullptr, *LoadTL);
		if (!Loaded)
		{
			UE_LOG(LogAnimStitchSpike, Error, TEXT("LoadTL failed"));
			return 5;
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("LoadTL: %d clips, authority %d, playback %.2f..%.2f"), Loaded->Timeline.Clips.Num(), Loaded->Timeline.RootAuthorityIndex, Loaded->PlaybackStart, Loaded->PlaybackEnd);
		for (const FAnimStitchClip& K : Loaded->Timeline.Clips)
		{
			UE_LOG(LogAnimStitchSpike, Display, TEXT("  %s trim %.2f-%.2f overlap %.2f batch %d  src %.1f fps, %d frames"), *GetNameSafe(K.Sequence), K.TrimIn, K.TrimOut, K.OverlapIn, K.bBatchSlot, K.Sequence ? K.Sequence->GetDataModel()->GetFrameRate().AsDecimal() : 0.0, K.Sequence ? K.Sequence->GetDataModel()->GetNumberOfFrames() : 0);
		}
		return 0;
	}
	if (FParse::Value(*Params, TEXT("Batch="), BatchFolder) && FParse::Value(*Params, TEXT("D="), PathD))
	{
		UAnimSequence* D = LoadObject<UAnimSequence>(nullptr, *PathD);
		const FString TLPath = BatchFolder / TEXT("TL_BatchTemplate");
		UPackage* Pkg = CreatePackage(*TLPath);
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(Pkg, TEXT("TL_BatchTemplate"), RF_Public | RF_Standalone | RF_Transactional);
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		TLAsset->SetTrim(1, 0.0, 1.5);
		TLAsset->SetOverlap(1, 0.3);
		TLAsset->SetBatchSlot(1, true);
		TLAsset->SetRootAuthority(1);
		TLAsset->MarkPackageDirty();
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Pkg, TLAsset, *FPackageName::LongPackageNameToFilename(TLPath, FPackageName::GetAssetPackageExtension()), SaveArgs);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("Batch: template saved=%d"), bSaved);

		FAnimStitchBatchOptions Opt;
		Opt.OutputFolder = BatchFolder;
		Opt.FrameRate = 30;
		const TArray<TArray<UAnimSequence*>> Assign = { { const_cast<UAnimSequence*>(B), D } };
		const FAnimStitchBatchResult R = FAnimStitchBatch::Run(TLAsset->Timeline, Assign, Opt);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("Batch: created %d, failed %d"), R.CreatedAssets.Num(), R.Failures.Num());
		for (const FString& P : R.CreatedAssets)
		{
			UAnimSequence* BakedOut = LoadObject<UAnimSequence>(nullptr, *(P + TEXT(".") + FPackageName::GetShortName(P)));
			UE_LOG(LogAnimStitchSpike, Display, TEXT("  %s -> %.3fs, %d tracks"), *P, BakedOut ? BakedOut->GetPlayLength() : -1.f, BakedOut ? BakedOut->GetDataModel()->GetNumBoneTracks() : -1);
		}
		for (const FText& F : R.Failures) { UE_LOG(LogAnimStitchSpike, Warning, TEXT("  failure: %s"), *F.ToString()); }
		FText ValidateErr;
		UE_LOG(LogAnimStitchSpike, Display, TEXT("Batch: uneven validation -> %d"), FAnimStitchBatch::Validate(TLAsset->Timeline, { { const_cast<UAnimSequence*>(B), D }, { const_cast<UAnimSequence*>(B) } }, ValidateErr));
		return R.Failures.Num() == 0 ? 0 : 6;
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
	FAnimStitchBakeOptions BakeOpts;
	BakeOpts.FrameRate = Fps > 0 ? FFrameRate(Fps, 1) : FFrameRate(0, 1);
	UAnimSequence* Baked = FAnimStitchBaker::Bake(TL, Out, BakeOpts, Error);
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










