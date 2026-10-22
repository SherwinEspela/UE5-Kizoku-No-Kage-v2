#include "AnimStitchSpikeCommandlet.h"
#include "AnimStitchBaker.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchBatch.h"
#include "AnimStitchBlueprintLibrary.h"
#include "AnimStitchFootLock.h"
#include "AnimStitchManualEdits.h"
#include "AnimStitchPreview.h"
#include "AnimStitchPoseComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Algo/MaxElement.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "ReferenceSkeleton.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "ScopedTransaction.h"

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

	// Phase 18 test: -P18Test with -A=<landing> -B=<run>. Two-way trim, per-clip play rate, partial-range bake.
	if (FParse::Param(*Params, TEXT("P18Test")))
	{
		const FString Folder = TEXT("/Game/_AnimStitchSpike");
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		TLAsset->SetTrim(1, 0.0, FMath::Min(3.0, (double)B->GetPlayLength()));
		TLAsset->SetOverlap(1, 0.2);
		const double Full = B->GetPlayLength();

		// 1) trim down, then back out to the source's limits
		TLAsset->SetTrim(1, 0.5, 1.5);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P18 TRIM: shrunk to %.2f..%.2f"), TLAsset->Timeline.Clips[1].TrimIn, TLAsset->Timeline.Clips[1].TrimOut);
		TLAsset->SetTrim(1, 0.0, Full + 5.0);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P18 TRIM: dragged back out -> %.2f..%.2f (source %.2f s; expected 0..source)"), TLAsset->Timeline.Clips[1].TrimIn, TLAsset->Timeline.Clips[1].TrimOut, Full);
		TLAsset->SetTrim(1, 0.0, 3.0);

		// 2) play rate
		const double W0 = TLAsset->Timeline.Clips[1].GetDuration();
		FAnimStitchEvaluator E1; FString Er;
		E1.Initialize(TLAsset->Timeline, Er);
		const FTransform R0 = E1.EvaluateClipRoot(1, TLAsset->Timeline.Clips[1].StartTime), R1 = E1.EvaluateClipRoot(1, TLAsset->Timeline.Clips[1].GetEndTime());
		const double Disp1 = (R1.GetTranslation() - R0.GetTranslation()).Size();
		TLAsset->SetClipRate(1, 0.5);
		const FAnimStitchClip& C1 = TLAsset->Timeline.Clips[1];
		FAnimStitchEvaluator E2; E2.Initialize(TLAsset->Timeline, Er);
		const FTransform S0 = E2.EvaluateClipRoot(1, C1.StartTime), S1 = E2.EvaluateClipRoot(1, C1.GetEndTime());
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P18 RATE: 0.5x: width %.3f s -> %.3f s (x%.2f); source in/out unchanged %.2f..%.2f; root displacement %.2f -> %.2f cm (same distance over twice the time)"),
			W0, C1.GetDuration(), C1.GetDuration() / W0, C1.TrimIn, C1.TrimOut, Disp1, (S1.GetTranslation() - S0.GetTranslation()).Size());
		TLAsset->SetClipRate(1, 99.0);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P18 RATE: 99 clamps to %.2f; "), TLAsset->Timeline.Clips[1].PlayRate);
		TLAsset->SetClipRate(1, 0.0);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P18 RATE: 0 clamps to %.2f"), TLAsset->Timeline.Clips[1].PlayRate);
		TLAsset->SetClipRate(1, 0.5);

		// 3) partial-range bake
		FAnimStitchBakeOptions Full_Opt; Full_Opt.bSavePackage = false; Full_Opt.bOverwriteExisting = true; Full_Opt.FrameRate = FFrameRate(30, 1);
		FString BakeErr;
		UAnimSequence* FullSeq = FAnimStitchBaker::Bake(TLAsset->Timeline, Folder / TEXT("AS_P18Full"), Full_Opt, BakeErr);
		FAnimStitchBakeOptions Part_Opt = Full_Opt; Part_Opt.bPartialRange = true; Part_Opt.RangeStart = 1.0; Part_Opt.RangeEnd = 2.0;
		UAnimSequence* PartSeq = FAnimStitchBaker::Bake(TLAsset->Timeline, Folder / TEXT("AS_P18Part"), Part_Opt, BakeErr);
		if (!FullSeq || !PartSeq) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P18 BAKE failed: %s"), *BakeErr); return 90; }
		const FReferenceSkeleton& Ref = FullSeq->GetSkeleton()->GetReferenceSkeleton();
		auto ReadBone = [&](const UAnimSequence* Seq, int32 Frame, FName Bone) { return Seq->GetDataModel()->GetBoneTrackTransform(Bone, FFrameNumber(Frame)); };
		const FName Foot = TEXT("foot_l");
		double Worst = 0.0;
		for (int32 f = 0; f <= PartSeq->GetDataModel()->GetNumberOfFrames(); ++f)
		{
			Worst = FMath::Max(Worst, (ReadBone(PartSeq, f, Foot).GetTranslation() - ReadBone(FullSeq, 30 + f, Foot).GetTranslation()).Size());
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P18 BAKE: full %d frames; flagged 1.0-2.0 s -> %d frames (expected 30); partial frame k vs full frame 30+k (foot_l) max diff %.5f cm (starts at the range start)"),
			FullSeq->GetDataModel()->GetNumberOfFrames(), PartSeq->GetDataModel()->GetNumberOfFrames(), Worst);
		return 0;
	}
	// Phase 17 test: -P17Test with -A=<landing> -B=<run>. (1) Absolute track creation bakes a key on every frame and reproduces the animation; edits change only the touched frames.
	// (2) Gap crossfade: spacing continuum (overlap / cut / gap), evaluation at the span ends, bake == preview evaluation.
	if (FParse::Param(*Params, TEXT("P17Test")))
	{
		const FString Folder = TEXT("/Game/_AnimStitchSpike");
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		TLAsset->SetTrim(1, 0.0, FMath::Min(3.0, (double)B->GetPlayLength()));
		TLAsset->SetOverlap(1, 0.2);
		TLAsset->SetRootAuthority(0);
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
		auto CS = [&Ref](const TArray<FTransform>& Local, int32 Bone) { return FAnimStitchFootLock::GetComponentSpace(Ref, Local, Bone); };
		auto BakeAll = [&](const TCHAR* Name, TArray<TArray<FTransform>>& Out) -> bool
		{
			FString Err;
			UAnimSequence* Seq = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, Folder / Name, 30, true, Err);
			if (!Seq) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P17Test bake %s failed: %s"), Name, *Err); return false; }
			const int32 N = Seq->GetDataModel()->GetNumberOfFrames();
			Out.SetNum(N + 1);
			for (int32 f = 0; f <= N; ++f) { ReadPose(Seq, f, Out[f]); }
			return true;
		};
		const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
		const int32 FootL = Ref.FindBoneIndex(TEXT("foot_l"));
		if (Pelvis == INDEX_NONE || FootL == INDEX_NONE) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P17Test: pelvis/foot_l not found")); return 70; }

		// ---- 1) Absolute track: per-frame bake ----
		{
			TArray<TArray<FTransform>> Base;
			if (!BakeAll(TEXT("AS_P17Base"), Base)) { return 71; }
			const int32 NumFrames = Base.Num() - 1;
			FAnimStitchTrack Track;
			Track.Kind = EAnimStitchTrackKind::Bone;
			Track.Bone = TEXT("pelvis");
			Track.Mode = EAnimStitchTrackMode::Absolute;
			Track.bKeepFeetPlanted = false;
			const int32 TI = TLAsset->AddTrack(Track);
			const FAnimStitchTrack& T = TLAsset->Timeline.Tracks[TI];
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 ABS: new absolute track has %d keys for %d baked frames (+1 end key), first %.4f s, last %.4f s (timeline %.4f s)"),
				T.Keys.Num(), NumFrames, T.Keys[0].Time, T.Keys.Last().Time, TLAsset->Timeline.GetTotalLength());
			TArray<TArray<FTransform>> Untouched;
			if (!BakeAll(TEXT("AS_P17Abs"), Untouched)) { return 72; }
			double Worst = 0.0;
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				for (const int32 Bone : { Pelvis, FootL })
				{
					Worst = FMath::Max(Worst, (CS(Untouched[f], Bone).GetTranslation() - CS(Base[f], Bone).GetTranslation()).Size());
				}
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 ABS: no edits, every frame: bake vs underlying animation max difference %.5f cm (expected ~0)"), Worst);

			// Edit frames 40..44 (+20 cm up on the keys themselves).
			const int32 EF0 = 40, EF1 = FMath::Min(44, NumFrames - 2);
			for (int32 k = EF0; k <= EF1; ++k)
			{
				FAnimStitchTrackKey Key = TLAsset->Timeline.Tracks[TI].Keys[k];
				Key.Translation += FVector(0, 0, 20);
				TLAsset->SetTrackKey(TI, Key);
			}
			TArray<TArray<FTransform>> Edited;
			if (!BakeAll(TEXT("AS_P17AbsEdit"), Edited)) { return 73; }
			double Outside = 0.0, Inside = 1e9;
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				const double Diff = (CS(Edited[f], Pelvis).GetTranslation() - CS(Base[f], Pelvis).GetTranslation()).Size();
				if (f >= EF0 && f <= EF1) { Inside = FMath::Min(Inside, Diff); } else { Outside = FMath::Max(Outside, Diff); }
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 ABS: edited keys %d..%d: frames outside the edit differ by max %.5f cm (expected ~0, the edited frames' smallest change is %.2f cm)"), EF0, EF1, Outside, Inside);
			TLAsset->RemoveTrack(TI);
		}

		// ---- 2) Gap crossfade ----
		{
			const FAnimStitchClip& C0 = TLAsset->Timeline.Clips[0];
			const FAnimStitchClip& C1 = TLAsset->Timeline.Clips[1];
			TLAsset->SetOverlap(1, 0.2);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 GAP: overlap 0.2 -> clip1 starts %.3f (clip0 ends %.3f)"), C1.StartTime, C0.GetEndTime());
			TLAsset->SetOverlap(1, 0.0);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 GAP: cut -> clip1 starts %.3f (clip0 ends %.3f)"), C1.StartTime, C0.GetEndTime());
			TLAsset->SetOverlap(1, -0.5);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 GAP: gap 0.5 -> clip1 starts %.3f (clip0 ends %.3f), OverlapIn %.3f, total length %.3f"), C1.StartTime, C0.GetEndTime(), C1.OverlapIn, TLAsset->Timeline.GetTotalLength());

			FAnimStitchEvaluator E2;
			FString Err2;
			if (!E2.Initialize(TLAsset->Timeline, Err2)) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P17 GAP: evaluator init failed: %s"), *Err2); return 74; }
			const double GapStart = C0.GetEndTime(), GapEnd = C1.StartTime;
			TArray<FTransform> PA, PB, PM, PAend, PBstart;
			E2.EvaluatePose(GapStart - 0.01, PAend);
			E2.EvaluatePose(GapStart, PA);
			E2.EvaluatePose(GapEnd, PB);
			E2.EvaluatePose(GapEnd + 0.01, PBstart);
			E2.EvaluatePose((GapStart + GapEnd) * 0.5, PM);
			auto Dist = [&](const TArray<FTransform>& X, const TArray<FTransform>& Y, int32 Bone) { return (CS(X, Bone).GetTranslation() - CS(Y, Bone).GetTranslation()).Size(); };
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 GAP: evaluation at gap start vs end of clip 0 (foot_l) %.4f cm; at gap end vs start of clip 1 %.4f cm; mid-gap foot is %.2f cm from A's end / %.2f cm from B's start (A->B %.2f cm)"),
				Dist(PA, PAend, FootL), Dist(PB, PBstart, FootL), Dist(PM, PA, FootL), Dist(PM, PB, FootL), Dist(PA, PB, FootL));
			double MaxStep = 0.0, MaxStepTime = 0.0;
			TArray<FTransform> Prev, Cur;
			E2.EvaluatePose(0.0, Prev);
			for (double Time = 1.0 / 30.0; Time <= E2.GetTimeline().GetTotalLength(); Time += 1.0 / 30.0)
			{
				E2.EvaluatePose(Time, Cur);
				const double Step = Dist(Cur, Prev, 0); if (Step > MaxStep) { MaxStep = Step; MaxStepTime = Time; }
				Prev = Cur;
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 GAP: largest root step between frames across the whole timeline %.2f cm at %.3f s (gap %.3f..%.3f)"), MaxStep, MaxStepTime, GapStart, GapEnd);

			TArray<TArray<FTransform>> Baked;
			if (!BakeAll(TEXT("AS_P17Gap"), Baked)) { return 75; }
			double MaxDiff = 0.0;
			for (int32 f = 0; f < Baked.Num(); ++f)
			{
				TArray<FTransform> Pose;
				E2.EvaluatePose(f / 30.0, Pose);
				for (const int32 Bone : { 0, Pelvis, FootL })
				{
					MaxDiff = FMath::Max(MaxDiff, Dist(Pose, Baked[f], Bone));
				}
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P17 GAP: baked %d frames vs preview evaluation: max difference %.5f cm"), Baked.Num(), MaxDiff);
			TLAsset->SetOverlap(1, 0.2);
		}
		return 0;
	}
	// Phase 16 test: -P16Test with -A=<landing> -B=<run>. Absolute bone tracks (replace the animation across the keyed range, no pops, bake == evaluation)
	// and the Root offset track (evaluated with the blend before foot lock: planted feet stay glued on the corrected path).
	// Hotfix Set 2 test: per-channel keys and channel locks on a bone track (data level, no editor UI).
	// Hotfix Set 3 test: absolute tracks fill new clip spans with per-frame baseline keys; clip block move / multi remove.
	// Hotfix Set 5 test: -S5Test. Trail refresh, deletion vs channel locks, control parenting, Absolute delete-interpolation on crossfades.
	if (FParse::Param(*Params, TEXT("S5Test")))
	{
		int32 Fails = 0;
		auto Check = [&Fails](bool b, const TCHAR* What) { UE_LOG(LogAnimStitchSpike, Display, TEXT("S5 %s: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What); if (!b) { ++Fails; } };
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(B), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		FAnimStitchEvaluator Eval; FString Err;
		if (!Eval.Initialize(TLAsset->Timeline, Err)) { UE_LOG(LogAnimStitchSpike, Error, TEXT("S5: evaluator failed: %s"), *Err); return 61; }
		const FReferenceSkeleton& Ref = Eval.GetSkeleton()->GetReferenceSkeleton();

		// S5-2: deletion ignores channel locks (Translation locked by default).
		{
			FAnimStitchTrack Fk; Fk.Kind = EAnimStitchTrackKind::Bone; Fk.Bone = TEXT("spine_01");
			const int32 TI = TLAsset->AddTrack(Fk);
			Check(!(TLAsset->Timeline.Tracks[TI].UnlockedChannels & AnimStitchChannel::Translation), TEXT("translation lane is locked by default"));
			FAnimStitchTrackKey K; K.Time = 1.0; K.Channels = AnimStitchChannel::All; K.Translation = FVector(1, 2, 3);
			FAnimStitchTrack& T = TLAsset->Timeline.Tracks[TI];
			T.Keys.Reset(); T.Keys.Add(K);
			K.Time = 2.0; T.Keys.Add(K);
			TLAsset->RemoveTrackChannelKey(TI, 0, AnimStitchChannel::Translation);
			Check(T.Keys.Num() == 2 && !(T.Keys[0].Channels & AnimStitchChannel::Translation) && (T.Keys[0].Channels & AnimStitchChannel::Rotation), TEXT("a locked translation lane key can be deleted"));
			TLAsset->RemoveTrackChannelKey(TI, 1, AnimStitchChannel::All);
			Check(T.Keys.Num() == 1, TEXT("the combined key deletes although translation / scale are locked"));
			TLAsset->RemoveTrackChannelKey(TI, 0, AnimStitchChannel::All);
			Check(T.Keys.Num() == 0, TEXT("a partially keyed combined key deletes"));
		}

		// S5-3: new controls parent to their skeleton parent's control.
		{
			FAnimStitchTrack Pelvis; Pelvis.Kind = EAnimStitchTrackKind::Bone; Pelvis.Bone = TEXT("pelvis"); Pelvis.Mode = EAnimStitchTrackMode::Absolute; Pelvis.bKeepFeetPlanted = false;
			TLAsset->AddTrack(Pelvis);
			FAnimStitchTrack Spine; Spine.Kind = EAnimStitchTrackKind::Bone; Spine.Bone = TEXT("spine_01"); Spine.Mode = EAnimStitchTrackMode::Absolute;
			const int32 SpineTrack = TLAsset->Timeline.Tracks.IndexOfByPredicate([](const FAnimStitchTrack& T) { return T.Bone == TEXT("spine_01"); });
			Check(SpineTrack != INDEX_NONE, TEXT("spine track exists"));
			// spine_01 existed before pelvis: it was created unparented; remove and re-add so it is created under the pelvis control.
			TLAsset->RemoveTrack(SpineTrack);
			const int32 NewSpine = TLAsset->AddTrack(Spine);
			const int32 PelvisTrack = TLAsset->Timeline.Tracks.IndexOfByPredicate([](const FAnimStitchTrack& T) { return T.Bone == TEXT("pelvis"); }); // indices shifted by the removal
			Check(TLAsset->Timeline.Tracks[NewSpine].ParentBone == TEXT("pelvis"), TEXT("spine_01 control is parented to the pelvis control"));
			FAnimStitchTrack Hand; Hand.Kind = EAnimStitchTrackKind::Bone; Hand.Bone = TEXT("hand_l");
			const int32 HandTrack = TLAsset->AddTrack(Hand);
			Check(TLAsset->Timeline.Tracks[HandTrack].ParentBone.IsNone(), TEXT("no control on the skeleton parent: stays top level"));
			Check(TLAsset->Timeline.Tracks[PelvisTrack].ParentBone.IsNone(), TEXT("the pelvis (its parent has no control) is top level"));
			Check(TLAsset->Timeline.Tracks.Num() == 3, TEXT("the track list is flat (3 tracks, no nesting)"));

			// Inheritance: lift the pelvis control; the Absolute child follows only while parented.
			const int32 SpineBone = Ref.FindBoneIndex(TEXT("spine_01"));
			auto SpineRel = [&](double Time)
			{
				TArray<FTransform> Pose; Eval.EvaluatePose(Time, Pose);
				FAnimStitchManualEdits::Apply(Ref, TLAsset->Timeline.Tracks, Time, Pose);
				return FAnimStitchFootLock::GetComponentSpace(Ref, Pose, SpineBone).GetRelativeTransform(FAnimStitchFootLock::GetComponentSpace(Ref, Pose, 0)).GetTranslation();
			};
			const FVector Before = SpineRel(0.8);
			for (FAnimStitchTrackKey& K : TLAsset->Timeline.Tracks[PelvisTrack].Keys) { K.Translation += FVector(0, 0, 30); }
			const FVector Carried = SpineRel(0.8);
			TLAsset->Timeline.Tracks[NewSpine].ParentBone = NAME_None;
			const FVector Free = SpineRel(0.8);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("S5 spine before %s, parented after lifting pelvis 30: %s, unparented: %s"), *Before.ToCompactString(), *Carried.ToCompactString(), *Free.ToCompactString());
			Check((Carried - Before - FVector(0, 0, 30)).Size() < 0.5, TEXT("moving the parent control carries the child control"));
			Check((Free - Before).Size() < 0.5, TEXT("an unparented Absolute control stays put"));
			TLAsset->RemoveTrack(HandTrack);
		}

		// S5-4: delete-interpolation across crossfades (XOver and gap), both curves.
		for (const EAnimStitchBlendCurve Curve : { EAnimStitchBlendCurve::Linear, EAnimStitchBlendCurve::Smoothstep })
		{
			for (const double XOver : { 0.4, -0.3 })
			{
				UAnimStitchTimelineAsset* A2 = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
				A2->AddSequences({ const_cast<UAnimSequence*>(B), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
				A2->Timeline.Clips[1].BlendIn = Curve;
				A2->SetOverlap(1, XOver);
				FAnimStitchTrack Abs; Abs.Kind = EAnimStitchTrackKind::Bone; Abs.Bone = TEXT("pelvis"); Abs.Mode = EAnimStitchTrackMode::Absolute; Abs.bKeepFeetPlanted = false;
				const int32 TI = A2->AddTrack(Abs);
				FAnimStitchTrack& T = A2->Timeline.Tracks[TI];
				const double JoinStart = A2->Timeline.Clips[1].StartTime;
				const double Lo = JoinStart - 0.25, Hi = JoinStart + 0.55; // a run that spans the crossfade
				T.Keys.RemoveAll([Lo, Hi](const FAnimStitchTrackKey& K) { return K.Time > Lo && K.Time < Hi; });
				auto WidestGap = [&](int32& OutIndex)
				{
					double Best = 0.0; OutIndex = INDEX_NONE;
					for (int32 i = 0; i + 1 < T.Keys.Num(); ++i) { if (T.Keys[i + 1].Time - T.Keys[i].Time > Best) { Best = T.Keys[i + 1].Time - T.Keys[i].Time; OutIndex = i; } }
					return Best;
				};
				int32 GapIndex = INDEX_NONE;
				const double GapBefore = WidestGap(GapIndex);
				const FString Label = FString::Printf(TEXT("%s, %s crossfade"), Curve == EAnimStitchBlendCurve::Linear ? TEXT("Linear") : TEXT("Smoothstep"), XOver > 0 ? TEXT("XOver") : TEXT("gap"));
				bool bOk = GapBefore > 0.7;
				// Dragging the crossfade length used to refill the deleted span from the clip.
				for (const double NewOverlap : { XOver * 0.5, XOver * 1.4, XOver })
				{
					A2->SetOverlap(1, NewOverlap);
					const double Gap = WidestGap(GapIndex);
					bOk &= Gap > 0.6;
					if (GapIndex != INDEX_NONE)
					{
						const FAnimStitchTrackKey& Ka = T.Keys[GapIndex]; const FAnimStitchTrackKey& Kb = T.Keys[GapIndex + 1];
						const double Mid = (Ka.Time + Kb.Time) * 0.5;
						const FVector Expected = FMath::Lerp(Ka.Translation, Kb.Translation, 0.5);
						bOk &= FAnimStitchManualEdits::Sample(T, Mid).Translation.Equals(Expected, 1e-3);
					}
				}
				Check(bOk, *(TEXT("deleted span stays empty and interpolates: ") + Label));
				// Edge: delete the head keys; the head holds the first surviving key, also after a crossfade edit.
				T.Keys.RemoveAll([](const FAnimStitchTrackKey& K) { return K.Time < 0.4; });
				A2->SetOverlap(1, XOver * 0.8);
				Check(T.Keys.Num() > 0 && T.Keys[0].Time > 0.3 && FAnimStitchManualEdits::Sample(T, 0.0).Translation.Equals(T.Keys[0].Translation, 1e-3), *(TEXT("deleted head holds the first surviving key: ") + Label));
			}
		}

		// S5-1: the trail follows every key change with no panel refresh in between.
		{
			USkeletalMesh* PreviewMesh = Eval.GetSkeleton()->GetPreviewMesh(true);
			if (!PreviewMesh) { UE_LOG(LogAnimStitchSpike, Error, TEXT("S5: no preview mesh")); return 62; }
			FAnimStitchTrack IK; IK.Kind = EAnimStitchTrackKind::IKLimb; IK.Bone = TEXT("foot_l"); IK.bShowTrail = true;
			const int32 IKTrack = TLAsset->AddTrack(IK);
			for (const double Time : { 0.2, 0.6, 1.0, 1.4 }) { FAnimStitchTrackKey K; K.Time = Time; K.Weight = 1.f; K.Translation = FVector(Time * 100.0, 10.0, 20.0); K.Pole = FVector(Time * 100.0, 40.0, 50.0); TLAsset->SetTrackKey(IKTrack, K); }
			TSharedRef<FAnimStitchPreview> Preview = MakeShared<FAnimStitchPreview>();
			Preview->SetMesh(PreviewMesh);
			Preview->SetAsset(TLAsset);
			Preview->SetTimeline(TLAsset->Timeline);
			const FAnimStitchControlId Id = { EAnimStitchControlKind::IKEffector, IKTrack, INDEX_NONE };
			const int32 Start = Preview->GetNumTrailKeysFor(Id);
			Check(Start >= 4, TEXT("trail shows its keys"));
			const int32 MidKey = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[IKTrack], 0.6);
			TLAsset->RemoveTrackKey(IKTrack, MidKey);
			Check(Preview->GetNumTrailKeysFor(Id) == Start - 1, TEXT("deleting a key removes its point at once"));
			FAnimStitchTrackKey Added; Added.Time = 0.8; Added.Weight = 1.f; Added.Translation = FVector(80, 10, 20);
			TLAsset->SetTrackKey(IKTrack, Added);
			Check(Preview->GetNumTrailKeysFor(Id) == Start, TEXT("adding a key adds its point"));
			const FVector* Before = nullptr; (void)Before;
			TLAsset->SetTrackKey(IKTrack, [&] { FAnimStitchTrackKey K = Added; K.Translation = FVector(80, 10, 90); return K; }());
			Preview->SelectControl(Id);
			const int32 PointIdx = Preview->FindTrailKey(0.8);
			Check(PointIdx != INDEX_NONE, TEXT("the edited key has a point"));
			const int32 NewIdx = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[IKTrack], 0.8);
			TLAsset->MoveTrackKey(IKTrack, NewIdx, 0.9);
			Check(Preview->FindTrailKey(0.9) != INDEX_NONE && Preview->FindTrailKey(0.8) == INDEX_NONE, TEXT("moving a key moves its point"));
			TLAsset->RemoveTrackKey(IKTrack, FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[IKTrack], 0.9));
			Check(Preview->GetNumTrailKeysFor(Id) == Start - 1 && Preview->FindTrailKey(0.9) == INDEX_NONE, TEXT("deleting again leaves no stale point"));
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("S5 RESULT: %d failures"), Fails);
		return Fails;
	}
	// Hotfix Set 4 test: -S4Test. Absolute tracks interpolate / hold (never fall back to the clip); per-control trail toggle.
	if (FParse::Param(*Params, TEXT("S4Test")))
	{
		int32 Fails = 0;
		auto Check = [&Fails](bool b, const TCHAR* What) { UE_LOG(LogAnimStitchSpike, Display, TEXT("S4 %s: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What); if (!b) { ++Fails; } };
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(B), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		FAnimStitchTrack Abs; Abs.Kind = EAnimStitchTrackKind::Bone; Abs.Bone = TEXT("pelvis"); Abs.Mode = EAnimStitchTrackMode::Absolute; Abs.bKeepFeetPlanted = false;
		const int32 AbsTrack = TLAsset->AddTrack(Abs);
		// Two surviving keys (everything between and around them "deleted").
		FAnimStitchTrack& AT = TLAsset->Timeline.Tracks[AbsTrack];
		AT.Keys.Reset();
		for (const double Time : { 0.5, 2.0 })
		{
			FAnimStitchTrackKey K; K.Time = Time; K.Channels = AnimStitchChannel::All;
			K.Translation = Time < 1.0 ? FVector(0, 0, 100) : FVector(40, 0, 140);
			AT.Keys.Add(K);
		}
		FAnimStitchEvaluator Eval; FString Err;
		if (!Eval.Initialize(TLAsset->Timeline, Err)) { UE_LOG(LogAnimStitchSpike, Error, TEXT("S4: evaluator failed: %s"), *Err); return 51; }
		const FReferenceSkeleton& Ref = Eval.GetSkeleton()->GetReferenceSkeleton();
		const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
		auto RelAt = [&](double Time)
		{
			TArray<FTransform> Pose;
			Eval.EvaluatePose(Time, Pose);
			FAnimStitchManualEdits::Apply(Ref, TLAsset->Timeline.Tracks, Time, Pose);
			return FAnimStitchFootLock::GetComponentSpace(Ref, Pose, Pelvis).GetRelativeTransform(FAnimStitchFootLock::GetComponentSpace(Ref, Pose, 0)).GetTranslation();
		};
		const FVector Head = RelAt(0.1), Mid = RelAt(1.25), Tail = RelAt(2.8);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("S4 head %s, middle %s, tail %s"), *Head.ToCompactString(), *Mid.ToCompactString(), *Tail.ToCompactString());
		Check(Head.Equals(FVector(0, 0, 100), 0.01), TEXT("the head span holds the first surviving key"));
		Check(Mid.Equals(FVector(20, 0, 120), 0.5), TEXT("the gap glides between the two surviving keys (midpoint)"));
		Check(Tail.Equals(FVector(40, 0, 140), 0.01), TEXT("the tail span holds the last surviving key"));
		Check(FAnimStitchManualEdits::IsActiveAt(AT, 0.0) && FAnimStitchManualEdits::IsActiveAt(AT, 3.2), TEXT("the track is active over the whole timeline"));

		// Per-control trail toggle.
		USkeletalMesh* PreviewMesh = Eval.GetSkeleton()->GetPreviewMesh(true);
		if (!PreviewMesh) { UE_LOG(LogAnimStitchSpike, Error, TEXT("S4: no preview mesh")); return 52; }
		FAnimStitchTrack IK; IK.Kind = EAnimStitchTrackKind::IKLimb; IK.Bone = TEXT("foot_l"); IK.bShowTrail = true;
		const int32 IKTrack = TLAsset->AddTrack(IK);
		for (const double Time : { 0.2, 0.6, 1.0 }) { FAnimStitchTrackKey K; K.Time = Time; K.Weight = 1.f; K.Translation = FVector(Time * 100.0, 10.0, 20.0); K.Pole = FVector(Time * 100.0, 40.0, 50.0); TLAsset->SetTrackKey(IKTrack, K); }
		FAnimStitchTrack Root; Root.Kind = EAnimStitchTrackKind::RootOffset; Root.Bone = Ref.GetBoneName(0); Root.bShowTrail = true;
		const int32 RootTrack = TLAsset->AddTrack(Root);
		for (const double Time : { 0.0, 0.5, 1.0 }) { FAnimStitchTrackKey K; K.Time = Time; K.Translation = FVector(0.0, Time * 50.0, 0.0); TLAsset->SetTrackKey(RootTrack, K); }
		TSharedRef<FAnimStitchPreview> Preview = MakeShared<FAnimStitchPreview>();
		Preview->SetMesh(PreviewMesh);
		Preview->SetAsset(TLAsset);
		Preview->SetTimeline(TLAsset->Timeline);
		const FAnimStitchControlId IKId = { EAnimStitchControlKind::IKEffector, IKTrack, INDEX_NONE };
		const FAnimStitchControlId RootId = { EAnimStitchControlKind::Bone, RootTrack, INDEX_NONE };
		Check(Preview->GetNumTrailKeysFor(IKId) > 0 && Preview->GetNumTrailKeysFor(RootId) > 0, TEXT("two controls show trails at once"));
		TLAsset->SetTrackShowTrail(IKTrack, false);
		Preview->SetManualTracks(TLAsset->Timeline.Tracks);
		Check(Preview->GetNumTrailKeysFor(IKId) == 0 && Preview->GetNumTrailKeysFor(RootId) > 0, TEXT("switching one control's trail off leaves the other visible"));
		Preview->ToggleHelpers();
		Check(Preview->GetNumTrailKeysFor(RootId) == 0, TEXT("G hides every trail"));
		Preview->ToggleHelpers();
		Check(Preview->GetNumTrailKeysFor(RootId) > 0 && Preview->GetNumTrailKeysFor(IKId) == 0, TEXT("G restores the trails that are switched on"));
		TLAsset->SetTrackShowTrail(IKTrack, true);
		Preview->SetManualTracks(TLAsset->Timeline.Tracks);
		Check(Preview->GetNumTrailKeysFor(IKId) > 0, TEXT("switching it back on shows it again"));
		UE_LOG(LogAnimStitchSpike, Display, TEXT("S4 RESULT: %d failures"), Fails);
		return Fails;
	}
	// Phase 20 test: -P20Test with -A=<landing> -B=<run>. Ground Feet pass, motion-trail points, undo.
	if (FParse::Param(*Params, TEXT("P20Test")))
	{
		int32 Fails = 0;
		auto Check = [&Fails](bool b, const TCHAR* What) { UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 %s: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What); if (!b) { ++Fails; } };
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TLAsset->SetFlags(RF_Transactional);
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(B), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W); // a run: plenty of plants
		TLAsset->Timeline.FootLock.HeightThreshold = 15.f;
		TLAsset->Timeline.FootLock.VelocityThreshold = 150.f;
		// Plants are forced with manual locks (the run clip itself barely plants): the test is of the pass, not of detection.
		for (const int32 Foot : { 0, 1 }) { TLAsset->AddFootEvent(Foot, 0.3, EAnimStitchFootAction::Lock); TLAsset->AddFootEvent(Foot, 0.9, EAnimStitchFootAction::Unlock); }
		TLAsset->AddFootEvent(0, 2.0, EAnimStitchFootAction::Lock); TLAsset->AddFootEvent(0, 2.6, EAnimStitchFootAction::Unlock);
		FAnimStitchEvaluator Eval; FString Err;
		if (!Eval.Initialize(TLAsset->Timeline, Err)) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P20: evaluator failed: %s"), *Err); return 41; }
		const FReferenceSkeleton& Ref = Eval.GetSkeleton()->GetReferenceSkeleton();
		constexpr double Fps = 30.0;
		const int32 NumFrames = FMath::RoundToInt(TLAsset->Timeline.GetTotalLength() * Fps) + 1;
		TArray<TArray<FTransform>> Poses; Poses.SetNum(NumFrames);
		for (int32 f = 0; f < NumFrames; ++f) { Eval.EvaluatePose(f / Fps, Poses[f]); }
		const FAnimStitchFootLockResult Det = FAnimStitchFootLock::Detect(Eval, Poses, Fps);
		Check(Det.bValid, TEXT("plant detection runs"));
		if (!Det.bValid) { return 42; }

		const TArray<FTransform>& RefPose = Ref.GetRefBonePose();
		const double RefAnkleZ = FMath::Min(FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Det.Chains[0].Foot).GetTranslation().Z, FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Det.Chains[1].Foot).GetTranslation().Z);
		const double FloorZ = Det.GroundZ - RefAnkleZ;
		// Lowest sole point of a foot (foot bone + toe bones, each with the reference floor point below it).
		auto LowestSole = [&](const TArray<FTransform>& Pose, int32 Foot)
		{
			double Lowest = TNumericLimits<double>::Max();
			for (int32 Bone = Det.Chains[Foot].Foot; Bone < Ref.GetNum(); ++Bone)
			{
				const int32 Parent = Ref.GetParentIndex(Bone);
				if (Bone != Det.Chains[Foot].Foot && !(Parent == Det.Chains[Foot].Foot || (Parent != INDEX_NONE && Ref.GetParentIndex(Parent) == Det.Chains[Foot].Foot))) { continue; }
				const FTransform RefCS = FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Bone);
				const FVector Local = RefCS.InverseTransformPosition(FVector(RefCS.GetTranslation().X, RefCS.GetTranslation().Y, 0.0));
				Lowest = FMath::Min(Lowest, FAnimStitchFootLock::GetComponentSpace(Ref, Pose, Bone).TransformPosition(Local).Z);
			}
			return Lowest;
		};
		auto TiltDegrees = [&](const TArray<FTransform>& Pose, int32 Foot)
		{
			const FTransform RefCS = FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Det.Chains[Foot].Foot);
			const FVector Up = FAnimStitchFootLock::GetComponentSpace(Ref, Pose, Det.Chains[Foot].Foot).GetRotation().RotateVector(RefCS.GetRotation().UnrotateVector(FVector::UpVector));
			return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Up.Z, -1.0, 1.0)));
		};
		auto HeadingOf = [&](const TArray<FTransform>& Pose, int32 Foot)
		{
			const FTransform RefCS = FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Det.Chains[Foot].Foot);
			const FVector Forward = FAnimStitchFootLock::GetComponentSpace(Ref, Pose, Det.Chains[Foot].Foot).GetRotation().RotateVector(RefCS.GetRotation().UnrotateVector(FVector::ForwardVector));
			return FMath::RadiansToDegrees(FMath::Atan2(Forward.Y, Forward.X));
		};
		// The first sustained plant of each foot (the run has several per foot).
		auto FindPlant = [&](int32 Foot, int32 After, int32& OutStart, int32& OutEnd)
		{
			for (int32 f = After; f < NumFrames; ++f)
			{
				if (!AnimStitchPlant::IsPlanted(Det.Mask[Foot][f])) { continue; }
				int32 E = f; while (E + 1 < NumFrames && Det.Mask[Foot][E + 1] == Det.Mask[Foot][f]) { ++E; }
				if (E - f >= 4) { OutStart = f; OutEnd = E; return true; }
				f = E;
			}
			return false;
		};
		int32 LS = 0, LE = 0, RS = 0, RE = 0;
		int32 PlantedL = 0, PlantedR = 0;
		for (int32 f = 0; f < NumFrames; ++f) { PlantedL += AnimStitchPlant::IsPlanted(Det.Mask[0][f]); PlantedR += AnimStitchPlant::IsPlanted(Det.Mask[1][f]); }
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 planted frames: left %d, right %d of %d (ground z %.2f)"), PlantedL, PlantedR, NumFrames, Det.GroundZ);
		const bool bHavePlants = FindPlant(0, 0, LS, LE) && FindPlant(1, 0, RS, RE);
		Check(bHavePlants, TEXT("the run has sustained plants on both feet"));
		if (!bHavePlants) { return 43; }
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 plants: left %d-%d, right %d-%d, ground z %.2f, floor z %.2f"), LS, LE, RS, RE, Det.GroundZ, FloorZ);

		// Perturb: a 15 degree tilt on the left plant (accidental), a 40 degree tilt on the right plant (deliberate toe/heel), and a second left plant floating 6 cm.
		int32 FS = 0, FE = 0;
		const bool bHaveFloater = FindPlant(0, LE + 1, FS, FE);
		Check(bHaveFloater, TEXT("a second left plant exists for the floating case"));
		TArray<TArray<FTransform>> Perturbed = Poses;
		const FTransform RefFootCS[2] = { FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Det.Chains[0].Foot), FAnimStitchFootLock::GetComponentSpace(Ref, RefPose, Det.Chains[1].Foot) };
		// Absolute set-up: a foot at the reference (level) orientation, tilted by Degrees about the forward axis, ankle Lift cm above the ground height.
		auto SetFoot = [&](int32 Foot, int32 From, int32 To, double Degrees, double Lift)
		{
			for (int32 f = From; f <= To; ++f)
			{
				const FVector Now = FAnimStitchFootLock::GetComponentSpace(Ref, Perturbed[f], Det.Chains[Foot].Thigh).GetTranslation(); // straight below the hip: always in reach
				const FQuat NewRot = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees)) * RefFootCS[Foot].GetRotation();
				FAnimStitchFootLock::SolveLimb(Ref, Perturbed[f], Det.Chains[Foot], FVector(Now.X, Now.Y, Det.GroundZ + Lift), nullptr, 0.0, &NewRot);
			}
		};
		SetFoot(0, LS, LE, 15.0, 3.0);
		SetFoot(1, RS, RE, 40.0, 3.0);
		if (bHaveFloater) { SetFoot(0, FS, FE, 0.0, 6.0); }
		const int32 LM = (LS + LE) / 2, RM = (RS + RE) / 2, FM = (FS + FE) / 2;
		const double HeadingBefore = HeadingOf(Perturbed[LM], 0);
		const double RightTiltBefore = TiltDegrees(Perturbed[RM], 1);
		const double FloaterBefore = bHaveFloater ? LowestSole(Perturbed[FM], 0) - FloorZ : 0.0;
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 set-up: left tilt %.1f (want 15), right tilt %.1f (want 40), floater %.2f cm (want ~6)"), TiltDegrees(Perturbed[LM], 0), RightTiltBefore, FloaterBefore);
		TArray<TArray<FTransform>> Untouched = Perturbed;
		const int32 Corrected = FAnimStitchFootLock::GroundFeet(Eval, Perturbed, Det);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 ground feet: %d foot-frames corrected; left tilt %.1f -> %.1f deg, right tilt %.1f -> %.1f deg, floater height %.2f -> %.2f cm"),
			Corrected, TiltDegrees(Untouched[LM], 0), TiltDegrees(Perturbed[LM], 0), RightTiltBefore, TiltDegrees(Perturbed[RM], 1), FloaterBefore, bHaveFloater ? LowestSole(Perturbed[FM], 0) - FloorZ : 0.0);
		Check(Corrected > 0, TEXT("the pass corrected foot-frames"));
		Check(TiltDegrees(Perturbed[LM], 0) < 1.0, TEXT("the 15 degree plant is levelled"));
		Check(FMath::Abs(FMath::FindDeltaAngleDegrees(HeadingBefore, HeadingOf(Perturbed[LM], 0))) < 1.0, TEXT("levelling keeps the heading (yaw)"));
		Check(FMath::Abs(LowestSole(Perturbed[LM], 0) - FloorZ) < 0.2, TEXT("the levelled plant sits on the floor"));
		Check(FMath::Abs(TiltDegrees(Perturbed[RM], 1) - RightTiltBefore) < 1.0, TEXT("the 40 degree toe plant keeps its rotation"));
		Check(FMath::Abs(LowestSole(Perturbed[RM], 1) - FloorZ) < 0.2, TEXT("the toe plant is seated by height"));
		if (bHaveFloater)
		{
			Check(FloaterBefore > 3.0 && FMath::Abs(LowestSole(Perturbed[FM], 0) - FloorZ) < 0.2, TEXT("the floating plant is seated"));
		}
		int32 SwingChanged = 0, SwingFrames = 0;
		for (int32 f = 0; f < NumFrames; ++f)
		{
			for (int32 Foot = 0; Foot < 2; ++Foot)
			{
				if (AnimStitchPlant::IsPlanted(Det.Mask[Foot][f])) { continue; }
				++SwingFrames;
				const int32 Foot0 = Det.Chains[Foot].Foot;
				const bool bSame = Perturbed[f][Foot0].Equals(Untouched[f][Foot0], 1e-6) && Perturbed[f][Det.Chains[Foot].Calf].Equals(Untouched[f][Det.Chains[Foot].Calf], 1e-6);
				SwingChanged += bSame ? 0 : 1;
			}
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 swing foot-frames %d, changed %d"), SwingFrames, SwingChanged);
		Check(SwingFrames > 0 && SwingChanged == 0, TEXT("swing feet are untouched"));

		// Flag + undo: one transaction, one Ctrl+Z.
		if (GEditor && GEditor->Trans)
		{
			const int32 Before = GEditor->Trans->GetQueueLength();
			{
				const FScopedTransaction Tx(NSLOCTEXT("AnimStitch", "P20GroundTx", "Ground Feet"));
				TLAsset->SetGroundFeet(true);
			}
			Check(TLAsset->Timeline.bGroundFeet && GEditor->Trans->GetQueueLength() == Before + 1, TEXT("Ground Feet is one transaction"));
			GEditor->UndoTransaction();
			Check(!TLAsset->Timeline.bGroundFeet, TEXT("one Ctrl+Z switches the pass back off"));
		}
		else { UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 SKIP: no transaction buffer in this commandlet")); }

		// ---- Motion trails ----
		USkeletalMesh* PreviewMesh = Eval.GetSkeleton()->GetPreviewMesh(true);
		if (!PreviewMesh) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P20: no preview mesh")); return 44; }
		TSharedRef<FAnimStitchPreview> Preview = MakeShared<FAnimStitchPreview>();
		Preview->SetMesh(PreviewMesh);
		Preview->SetAsset(TLAsset);
		FAnimStitchTrack IK; IK.Kind = EAnimStitchTrackKind::IKLimb; IK.Bone = Ref.GetBoneName(Det.Chains[0].Foot); IK.bShowTrail = true;
		const int32 IKTrack = TLAsset->AddTrack(IK);
		for (const double Time : { 0.2, 0.6, 1.0 })
		{
			FAnimStitchTrackKey K; K.Time = Time; K.Weight = 1.f; K.Translation = FVector(Time * 100.0, 10.0, 20.0); K.Pole = FVector(Time * 100.0, 40.0, 50.0);
			TLAsset->SetTrackKey(IKTrack, K);
		}
		FAnimStitchTrack Fk; Fk.Kind = EAnimStitchTrackKind::Bone; Fk.Bone = TEXT("spine_01"); Fk.bShowTrail = true;
		const int32 FkTrack = TLAsset->AddTrack(Fk);
		FAnimStitchTrackKey FkKey; FkKey.Time = 0.5; FkKey.Rotation = FQuat(FVector::RightVector, 0.2);
		TLAsset->SetTrackChannelKey(FkTrack, FkKey, AnimStitchChannel::Rotation);
		FAnimStitchTrack Root; Root.Kind = EAnimStitchTrackKind::RootOffset; Root.Bone = Ref.GetBoneName(0); Root.bShowTrail = true;
		const int32 RootTrack = TLAsset->AddTrack(Root);
		for (const double Time : { 0.0, 0.5, 1.0 })
		{
			FAnimStitchTrackKey K; K.Time = Time; K.Translation = FVector(0.0, Time * 50.0, 0.0);
			TLAsset->SetTrackKey(RootTrack, K);
		}
		Preview->SetTimeline(TLAsset->Timeline);
		Preview->SetPlayhead(0.5);

		Preview->SelectControl({ EAnimStitchControlKind::IKEffector, IKTrack, INDEX_NONE });
		int32 ActiveIKKeys = 0; for (const FAnimStitchTrackKey& K : TLAsset->Timeline.Tracks[IKTrack].Keys) { ActiveIKKeys += K.Weight > 1e-3f ? 1 : 0; }
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 IK track keys %d, with IK on %d, trail points %d"), TLAsset->Timeline.Tracks[IKTrack].Keys.Num(), ActiveIKKeys, Preview->GetNumTrailKeys());
		Check(Preview->GetNumTrailKeys() == ActiveIKKeys && Preview->GetNumTrailKeys() >= 3 && Preview->IsTrailEditable(), TEXT("selecting a foot IK target draws an editable trail with a point at each of its keys"));
		const int32 MidKey = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[IKTrack], 0.6);
		const int32 EndKey = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[IKTrack], 1.0);
		const int32 NumIKKeys = TLAsset->Timeline.Tracks[IKTrack].Keys.Num();
		const FVector MidBefore = TLAsset->Timeline.Tracks[IKTrack].Keys[MidKey].Translation;
		const FVector EndBefore = TLAsset->Timeline.Tracks[IKTrack].Keys[EndKey].Translation;
		const int32 Queue = (GEditor && GEditor->Trans) ? GEditor->Trans->GetQueueLength() : 0;
		const bool bDragged = Preview->DebugDragTrailKey(Preview->FindTrailKey(0.6), FVector(0.0, 0.0, 12.0));
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 trail: dragged=%d keys=%d mid %s -> %s"), bDragged, TLAsset->Timeline.Tracks[IKTrack].Keys.Num(), *MidBefore.ToCompactString(), *TLAsset->Timeline.Tracks[IKTrack].Keys[MidKey].Translation.ToCompactString());
		const FVector MidAfter = TLAsset->Timeline.Tracks[IKTrack].Keys[MidKey].Translation;
		Check(bDragged && MidAfter.Equals(MidBefore + FVector(0, 0, 12.0), 1e-3) && TLAsset->Timeline.Tracks[IKTrack].Keys[EndKey].Translation.Equals(EndBefore, 1e-6) && TLAsset->Timeline.Tracks[IKTrack].Keys.Num() == NumIKKeys, TEXT("dragging the mid trail point moves only that key by the drag"));
		if (GEditor && GEditor->Trans)
		{
			Check(GEditor->Trans->GetQueueLength() == Queue + 1, TEXT("the trail drag is one undo transaction"));
			GEditor->UndoTransaction();
			Check(TLAsset->Timeline.Tracks[IKTrack].Keys[MidKey].Translation.Equals(MidBefore, 1e-3), TEXT("undo restores the key"));
		}
		Preview->SetManualTracks(TLAsset->Timeline.Tracks);
		int32 PoleTrack = TLAsset->Timeline.Tracks.IndexOfByPredicate([](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::IKPole; });
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 tracks %d, ik %d, pole lane %d, fk %d, root %d"), TLAsset->Timeline.Tracks.Num(), IKTrack, PoleTrack, FkTrack, RootTrack);
		Preview->SelectControl({ EAnimStitchControlKind::IKPole, PoleTrack != INDEX_NONE ? PoleTrack : IKTrack, INDEX_NONE });
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 pole trail keys %d editable %d (lane keys %d)"), Preview->GetNumTrailKeys(), Preview->IsTrailEditable(), PoleTrack != INDEX_NONE ? TLAsset->Timeline.Tracks[PoleTrack].Keys.Num() : -1);
		Check(Preview->GetNumTrailKeys() == NumIKKeys && Preview->IsTrailEditable(), TEXT("the pole vector has an editable trail too"));
		Preview->SelectControl({ EAnimStitchControlKind::Bone, RootTrack, INDEX_NONE });
		const bool bRootTrail = Preview->GetNumTrailKeys() == TLAsset->Timeline.Tracks[RootTrack].Keys.Num() && Preview->IsTrailEditable();
		const int32 RootMid = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[RootTrack], 0.5);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 root trail keys %d editable %d (track keys %d, kind %d)"), Preview->GetNumTrailKeys(), Preview->IsTrailEditable(), TLAsset->Timeline.Tracks[RootTrack].Keys.Num(), (int32)TLAsset->Timeline.Tracks[RootTrack].Kind);
		const FVector RootBefore = TLAsset->Timeline.Tracks[RootTrack].Keys[RootMid].Translation;
		const bool bRootDrag = bRootTrail && Preview->DebugDragTrailKey(Preview->FindTrailKey(0.5), FVector(5.0, 0.0, 0.0));
		Check(bRootTrail && bRootDrag && TLAsset->Timeline.Tracks[RootTrack].Keys[RootMid].Translation.Equals(RootBefore + FVector(5.0, 0, 0), 1e-3), TEXT("the Root motion offset trail is editable"));
		Preview->SelectControl({ EAnimStitchControlKind::Bone, FkTrack, INDEX_NONE });
		const FQuat FkRotBefore = TLAsset->Timeline.Tracks[FkTrack].Keys.Last().Rotation;
		Check(Preview->GetNumTrailKeys() >= 1 && !Preview->IsTrailEditable(), TEXT("an FK bone draws a view-only trail"));
		Check(Preview->FindTrailKey(0.5) != INDEX_NONE && !Preview->DebugDragTrailKey(Preview->FindTrailKey(0.5), FVector(0, 0, 10.0)) && TLAsset->Timeline.Tracks[FkTrack].Keys.Last().Rotation.Equals(FkRotBefore, 1e-6), TEXT("FK trail points refuse dragging"));
		UE_LOG(LogAnimStitchSpike, Display, TEXT("P20 RESULT: %d failures"), Fails);
		return Fails;
	}

	if (FParse::Param(*Params, TEXT("S3Test")))
	{
		int32 Fails = 0;
		auto Check = [&Fails](bool b, const TCHAR* What) { UE_LOG(LogAnimStitchSpike, Display, TEXT("S3 %s: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What); if (!b) { ++Fails; } };
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A) }, INDEX_NONE, W);
		FAnimStitchTrack Track;
		Track.Kind = EAnimStitchTrackKind::Bone;
		Track.Bone = TEXT("pelvis");
		Track.Mode = EAnimStitchTrackMode::Absolute;
		Track.bKeepFeetPlanted = false;
		const int32 TI = TLAsset->AddTrack(Track);
		const double OldTotal = TLAsset->Timeline.GetTotalLength();
		// Edit a key in the old span.
		FAnimStitchTrackKey Edit = FAnimStitchManualEdits::Sample(TLAsset->Timeline.Tracks[TI], OldTotal * 0.5);
		Edit.Time = OldTotal * 0.5;
		Edit.Translation += FVector(0, 0, 30);
		TLAsset->SetTrackKey(TI, Edit);
		const int32 EditIndex = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[TI], Edit.Time);
		const FVector EditedT = TLAsset->Timeline.Tracks[TI].Keys[EditIndex].Translation;
		const int32 KeysBefore = TLAsset->Timeline.Tracks[TI].Keys.Num();
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		const double NewTotal = TLAsset->Timeline.GetTotalLength();
		const FAnimStitchTrack& T = TLAsset->Timeline.Tracks[TI];
		double Rate = A->GetDataModel()->GetFrameRate().AsDecimal(); if (Rate < 1.0 || Rate > 120.0) { Rate = 30.0; }
		int32 Uncovered = 0;
		for (double t = OldTotal; t <= NewTotal; t += 1.0 / Rate)
		{
			bool bNear = false;
			for (const FAnimStitchTrackKey& K : T.Keys) { bNear |= FMath::Abs(K.Time - t) <= 0.75 / Rate; }
			Uncovered += bNear ? 0 : 1;
		}
		Check(NewTotal > OldTotal + 0.1 && T.Keys.Num() > KeysBefore + 5, TEXT("adding a clip adds per-frame keys"));
		Check(Uncovered == 0, TEXT("no frame of the new span is without a key"));
		Check(FAnimStitchManualEdits::FindKey(T, Edit.Time) != INDEX_NONE && T.Keys[FAnimStitchManualEdits::FindKey(T, Edit.Time)].Translation.Equals(EditedT, 1e-4), TEXT("the earlier edit is untouched"));
		// New-span keys == evaluated underlying animation.
		FAnimStitchEvaluator Eval; FString Err; Eval.Initialize(TLAsset->Timeline, Err);
		const FReferenceSkeleton& Ref = Eval.GetSkeleton()->GetReferenceSkeleton();
		const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
		double MaxDiff = 0.0; TArray<FTransform> Pose;
		for (const FAnimStitchTrackKey& K : T.Keys)
		{
			if (K.Time <= OldTotal + 0.5 / Rate) { continue; }
			Eval.EvaluatePose(K.Time, Pose);
			const FTransform Rel = FAnimStitchFootLock::GetComponentSpace(Ref, Pose, Pelvis).GetRelativeTransform(FAnimStitchFootLock::GetComponentSpace(Ref, Pose, 0));
			MaxDiff = FMath::Max(MaxDiff, (Rel.GetTranslation() - K.Translation).Size());
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("S3 new-span keys vs evaluated animation: max difference %.5f cm"), MaxDiff);
		Check(MaxDiff < 1e-3, TEXT("filled keys reproduce the new clip's motion"));
		// Block move and multi remove.
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W); // clips: A B A B
		TLAsset->EnsureClipIds();
		const FGuid Id0 = TLAsset->Timeline.Clips[0].ClipId, Id2 = TLAsset->Timeline.Clips[2].ClipId, Id1 = TLAsset->Timeline.Clips[1].ClipId, Id3 = TLAsset->Timeline.Clips[3].ClipId;
		TLAsset->MoveClipBlock({ 0, 2 }, 2);
		Check(TLAsset->Timeline.Clips[0].ClipId == Id1 && TLAsset->Timeline.Clips[1].ClipId == Id3 && TLAsset->Timeline.Clips[2].ClipId == Id0 && TLAsset->Timeline.Clips[3].ClipId == Id2, TEXT("moving clips 0 and 2 to the end keeps their order as a block"));
		TLAsset->RemoveClips({ 0, 1, 2 });
		Check(TLAsset->Timeline.Clips.Num() == 1 && TLAsset->Timeline.Clips[0].ClipId == Id2, TEXT("removing several clips at once leaves the right one"));
		UE_LOG(LogAnimStitchSpike, Display, TEXT("S3 RESULT: %d failures"), Fails);
		return Fails;
	}
	if (FParse::Param(*Params, TEXT("S2Test")))
	{
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		FAnimStitchTrack Track;
		Track.Kind = EAnimStitchTrackKind::Bone;
		Track.Bone = TEXT("spine_01");
		const int32 TI = TLAsset->AddTrack(Track);
		auto Count = [&](uint8 Bit) { int32 N = 0; for (const FAnimStitchTrackKey& K : TLAsset->Timeline.Tracks[TI].Keys) { N += (K.Channels & Bit) ? 1 : 0; } return N; };
		int32 Fails = 0;
		auto Check = [&Fails](bool b, const TCHAR* What) { UE_LOG(LogAnimStitchSpike, Display, TEXT("S2 %s: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What); if (!b) { ++Fails; } };
		Check(TLAsset->Timeline.Tracks[TI].UnlockedChannels == AnimStitchChannel::Rotation, TEXT("new track: only Rotation unlocked (Translation + Scale locked)"));
		const int32 T0 = Count(AnimStitchChannel::Translation), R0 = Count(AnimStitchChannel::Rotation);
		FAnimStitchTrackKey Key;
		Key.Time = 0.5;
		Key.Rotation = FQuat(FVector::UpVector, 0.3);
		Key.Translation = FVector(5, 0, 0);
		TLAsset->SetTrackChannelKey(TI, Key, AnimStitchChannel::Rotation);
		Check(Count(AnimStitchChannel::Rotation) == R0 + 1 && Count(AnimStitchChannel::Translation) == T0 && Count(AnimStitchChannel::Scale) == T0, TEXT("rotation key adds no translation / scale keys"));
		TLAsset->SetTrackChannelKey(TI, Key, AnimStitchChannel::Translation | AnimStitchChannel::Scale);
		Check(Count(AnimStitchChannel::Translation) == T0, TEXT("locked translation / scale refuse the write"));
		const FAnimStitchTrackKey S = FAnimStitchManualEdits::Sample(TLAsset->Timeline.Tracks[TI], 0.5);
		Check(S.Rotation.Equals(Key.Rotation, 1e-5) && S.Translation.IsNearlyZero(1e-4), TEXT("sample: rotation keyed, translation stays identity"));
		TLAsset->SetTrackChannelUnlocked(TI, AnimStitchChannel::Translation, true);
		TLAsset->SetTrackChannelKey(TI, Key, AnimStitchChannel::Translation);
		const int32 KI = FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[TI], 0.5);
		Check(Count(AnimStitchChannel::Translation) == T0 + 1 && KI != INDEX_NONE && TLAsset->Timeline.Tracks[TI].Keys[KI].Channels == (AnimStitchChannel::Translation | AnimStitchChannel::Rotation), TEXT("unlocked translation keys into the same key, scale untouched"));
		const int32 NI = TLAsset->MoveTrackChannelKey(TI, KI, AnimStitchChannel::Rotation, 0.8);
		Check(NI != INDEX_NONE && FMath::IsNearlyEqual(TLAsset->Timeline.Tracks[TI].Keys[NI].Time, 0.8) && TLAsset->Timeline.Tracks[TI].Keys[NI].Channels == AnimStitchChannel::Rotation && FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[TI], 0.5) != INDEX_NONE, TEXT("moving the rotation key leaves the translation key behind"));
		TLAsset->SetTrackChannelUnlocked(TI, AnimStitchChannel::Rotation, false);
		const double Before = TLAsset->Timeline.Tracks[TI].Keys[NI].Time;
		TLAsset->MoveTrackChannelKey(TI, NI, AnimStitchChannel::Rotation, 1.0);
		TLAsset->RemoveTrackChannelKey(TI, NI, AnimStitchChannel::Rotation);
		Check(Count(AnimStitchChannel::Rotation) == R0 && FMath::IsNearlyEqual(Before, 0.8), TEXT("locked rotation key cannot be moved, but deleting ignores the lock (Set 5)"));
		TLAsset->SetTrackChannelUnlocked(TI, AnimStitchChannel::Rotation, true);
		Check(Count(AnimStitchChannel::Rotation) == R0, TEXT("rotation key is gone after the lock-ignoring delete"));
		UE_LOG(LogAnimStitchSpike, Display, TEXT("S2 RESULT: %d failures"), Fails);
		return Fails;
	}
	if (FParse::Param(*Params, TEXT("P16Test")))
	{
		const FString Folder = TEXT("/Game/_AnimStitchSpike");
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		TLAsset->SetTrim(1, 0.0, FMath::Min(3.0, (double)B->GetPlayLength()));
		TLAsset->SetOverlap(1, 0.2);
		TLAsset->SetRootAuthority(0);
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
		auto CS = [&Ref](const TArray<FTransform>& Local, int32 Bone) { return FAnimStitchFootLock::GetComponentSpace(Ref, Local, Bone); };
		auto BakeAll = [&](const TCHAR* Name, TArray<TArray<FTransform>>& Out) -> bool
		{
			FString Err;
			UAnimSequence* Seq = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, Folder / Name, 30, true, Err);
			if (!Seq) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P16Test bake %s failed: %s"), Name, *Err); return false; }
			const int32 N = Seq->GetDataModel()->GetNumberOfFrames();
			Out.SetNum(N + 1);
			for (int32 f = 0; f <= N; ++f) { ReadPose(Seq, f, Out[f]); }
			return true;
		};
		const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
		const int32 FootL = Ref.FindBoneIndex(TEXT("foot_l"));
		if (Pelvis == INDEX_NONE || FootL == INDEX_NONE) { UE_LOG(LogAnimStitchSpike, Error, TEXT("P16Test: pelvis/foot_l not found")); return 60; }

		TArray<TArray<FTransform>> Base;
		if (!BakeAll(TEXT("AS_P16Base"), Base)) { return 61; }
		const int32 NumFrames = Base.Num() - 1;
		const double Total = TLAsset->Timeline.GetTotalLength();

		// ---- Absolute pelvis track ----
		{
			FAnimStitchTrack Track;
			Track.Kind = EAnimStitchTrackKind::Bone;
			Track.Bone = TEXT("pelvis");
			Track.Mode = EAnimStitchTrackMode::Absolute;
			Track.bKeepFeetPlanted = false;
			const int32 TI = TLAsset->AddTrack(Track);
			const FAnimStitchTrack& T = TLAsset->Timeline.Tracks[TI];
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ABS: new absolute track has %d boundary keys (times %.2f / %.2f s); mode=%d; default mode of a plain track=%d (Additive=0)"),
				T.Keys.Num(), T.Keys.Num() > 0 ? T.Keys[0].Time : -1.0, T.Keys.Num() > 1 ? T.Keys.Last().Time : -1.0, (int32)T.Mode, (int32)FAnimStitchTrack().Mode);
			const FTransform S0 = CS(Base[0], Pelvis).GetRelativeTransform(CS(Base[0], 0)); // keys are relative to the root bone
			const FTransform SE = CS(Base[NumFrames], Pelvis).GetRelativeTransform(CS(Base[NumFrames], 0));
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ABS: boundary keys == underlying blended value: start diff %.5f cm, end diff %.5f cm"),
				(T.Keys[0].Translation - S0.GetTranslation()).Size(), (T.Keys.Last().Translation - SE.GetTranslation()).Size());

			// Two mid poses: the underlying pelvis shifted up 30 cm / sideways 25 cm.
			const double T1 = 1.5, T2 = 2.5;
			const int32 F1 = FMath::RoundToInt(T1 * 30.0), F2 = FMath::RoundToInt(T2 * 30.0);
			FAnimStitchTrackKey K1, K2;
			FTransform Want1 = CS(Base[F1], Pelvis); Want1.AddToTranslation(FVector(0, 0, 30));
			FTransform Want2 = CS(Base[F2], Pelvis); Want2.AddToTranslation(FVector(0, 25, 0));
			const FTransform Rel1 = Want1.GetRelativeTransform(CS(Base[F1], 0)), Rel2 = Want2.GetRelativeTransform(CS(Base[F2], 0));
			K1.Time = T1; K1.Translation = Rel1.GetTranslation(); K1.Rotation = Rel1.GetRotation(); K1.Scale = Rel1.GetScale3D();
			K2.Time = T2; K2.Translation = Rel2.GetTranslation(); K2.Rotation = Rel2.GetRotation(); K2.Scale = Rel2.GetScale3D();
			TLAsset->SetTrackKey(TI, K1);
			TLAsset->SetTrackKey(TI, K2);
			TArray<TArray<FTransform>> Abs;
			if (!BakeAll(TEXT("AS_P16Abs"), Abs)) { return 62; }
			double DevKey1 = (CS(Abs[F1], Pelvis).GetTranslation() - Want1.GetTranslation()).Size();
			double DevKey2 = (CS(Abs[F2], Pelvis).GetTranslation() - Want2.GetTranslation()).Size();
			// The pose must travel with the root: pelvis-to-root distance stays about what the animation has (it used to blow up on a travelling character).
			double MaxDistAbs = 0.0, MaxDistBase = 0.0;
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				MaxDistAbs = FMath::Max(MaxDistAbs, (CS(Abs[f], Pelvis).GetTranslation() - CS(Abs[f], 0).GetTranslation()).Size());
				MaxDistBase = FMath::Max(MaxDistBase, (CS(Base[f], Pelvis).GetTranslation() - CS(Base[f], 0).GetTranslation()).Size());
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ABS: root travels %.0f cm over the timeline; max pelvis-to-root distance %.1f cm with the absolute track vs %.1f cm in the animation"), (CS(Base[NumFrames], 0).GetTranslation() - CS(Base[0], 0).GetTranslation()).Size(), MaxDistAbs, MaxDistBase);
			double MaxStep = 0.0; // continuity across the whole timeline (no pops at start, first key, last key, end)
			for (int32 f = 1; f <= NumFrames; ++f) { MaxStep = FMath::Max(MaxStep, (CS(Abs[f], Pelvis).GetTranslation() - CS(Abs[f - 1], Pelvis).GetTranslation()).Size()); }
			double MaxBaseStep = 0.0;
			for (int32 f = 1; f <= NumFrames; ++f) { MaxBaseStep = FMath::Max(MaxBaseStep, (CS(Base[f], Pelvis).GetTranslation() - CS(Base[f - 1], Pelvis).GetTranslation()).Size()); }
			// Preview parity: the evaluator + Apply path (what the viewport runs) against the bake.
			double MaxPreviewDiff = 0.0;
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				TArray<FTransform> Pose;
				Eval.EvaluatePose(f / 30.0, Pose);
				FAnimStitchManualEdits::Apply(Ref, TLAsset->Timeline.Tracks, f / 30.0, Pose, nullptr);
				MaxPreviewDiff = FMath::Max(MaxPreviewDiff, (CS(Pose, Pelvis).GetTranslation() - CS(Abs[f], Pelvis).GetTranslation()).Size());
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ABS: keyed 30 cm up at %.1f s and 25 cm sideways at %.1f s: bake hits the keys within %.5f / %.5f cm | max frame step %.2f cm (animation alone: %.2f) | preview evaluation vs bake: %.5f cm"),
				T1, T2, DevKey1, DevKey2, MaxStep, MaxBaseStep, MaxPreviewDiff);
			double MidReplaced = 0.0;
			for (int32 f = F1; f <= F2; ++f) { MidReplaced = FMath::Max(MidReplaced, (CS(Abs[f], Pelvis).GetTranslation() - CS(Base[f], Pelvis).GetTranslation()).Size()); }
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ABS: between the keys the animation is replaced (max difference to the animation %.2f cm)"), MidReplaced);

			// Without boundary keys the animation plays untouched outside the keyed range.
			TLAsset->RemoveTrackKey(TI, 0);
			TLAsset->RemoveTrackKey(TI, TLAsset->Timeline.Tracks[TI].Keys.Num() - 1);
			TArray<TArray<FTransform>> Open;
			if (!BakeAll(TEXT("AS_P16AbsOpen"), Open)) { return 63; }
			double Outside = 0.0, Inside = 0.0;
			for (int32 f = 0; f <= NumFrames; ++f)
			{
				const double Diff = (CS(Open[f], Pelvis).GetTranslation() - CS(Base[f], Pelvis).GetTranslation()).Size();
				if (f < F1 - 1 || f > F2 + 1) { Outside = FMath::Max(Outside, Diff); } else if (f >= F1 && f <= F2) { Inside = FMath::Max(Inside, Diff); }
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ABS: boundary keys removed: outside the keyed range max difference to the animation %.5f cm (expected 0), inside %.2f cm"), Outside, Inside);
			TLAsset->RemoveTrack(TI);
		}

		// ---- Root offset track ----
		{
			TLAsset->Timeline.FootLock.bEnabled = true;
			TArray<TArray<FTransform>> LockBase;
			if (!BakeAll(TEXT("AS_P16LockBase"), LockBase)) { return 64; }
			FAnimStitchTrack Root;
			Root.Kind = EAnimStitchTrackKind::RootOffset;
			Root.Bone = Ref.GetBoneName(0); Root.bShowTrail = true;
			const int32 RI = TLAsset->AddTrack(Root);
			const int32 RI2 = TLAsset->AddTrack(Root); // a second one is refused
			FAnimStitchTrackKey End;
			End.Time = Total; End.Translation = FVector(0, 40, 0); End.Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(25.0));
			TLAsset->SetTrackKey(RI, End);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ROOT: root track index %d, second add returns %d (same track), %d tracks in total, %d keys (zero at 0, 40 cm sideways + 25 deg yaw at %.2f s)"),
				RI, RI2, TLAsset->Timeline.Tracks.Num(), TLAsset->Timeline.Tracks[RI].Keys.Num(), Total);
			TArray<TArray<FTransform>> RootBake;
			if (!BakeAll(TEXT("AS_P16Root"), RootBake)) { return 65; }
			const FTransform R0 = CS(LockBase[NumFrames], 0), R1 = CS(RootBake[NumFrames], 0);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ROOT: last-frame root moved by (%s) cm and %.1f deg yaw compared with no offset"), *(R1.GetTranslation() - R0.GetTranslation()).ToCompactString(),
				FMath::RadiansToDegrees((R1.GetRotation() * R0.GetRotation().Inverse()).GetTwistAngle(FVector::UpVector)));

			// Plants on the CORRECTED path: detect on the evaluated poses, then measure foot slide inside every planted run of a baked result.
			auto MeasureSlide = [&](const TCHAR* Label, const FAnimStitchTimeline& Timeline, const TArray<TArray<FTransform>>& Baked)
			{
				FAnimStitchEvaluator EvalX;
				FString E2;
				EvalX.Initialize(Timeline, E2);
				TArray<TArray<FTransform>> Poses; Poses.SetNum(NumFrames + 1);
				for (int32 f = 0; f <= NumFrames; ++f) { EvalX.EvaluatePose(f / 30.0, Poses[f]); }
				const FAnimStitchFootLockResult Det = FAnimStitchFootLock::Detect(EvalX, Poses, 30.0);
				for (int32 Foot = 0; Foot < 2 && Det.bValid; ++Foot)
				{
					const int32 Bone = Det.Chains[Foot].Foot;
					double WorstLocked = 0.0, WorstUnlocked = 0.0;
					int32 Runs = 0, Frames = 0;
					for (int32 f = 0; f <= NumFrames;)
					{
						if (!AnimStitchPlant::IsPlanted(Det.Mask[Foot][f])) { ++f; continue; }
						int32 e = f;
						while (e + 1 <= NumFrames && AnimStitchPlant::IsPlanted(Det.Mask[Foot][e + 1])) { ++e; }
						if (e - f >= 12) // interior of the run: the first / last 4 frames are the 0.1 s blend in / out of the pin
						{
							++Runs; const int32 f0 = f + 4, e0 = e - 4;
							FVector MeanLocked = FVector::ZeroVector, MeanUnlocked = FVector::ZeroVector;
							for (int32 k = f0; k <= e0; ++k) { MeanLocked += CS(Baked[k], Bone).GetTranslation(); MeanUnlocked += CS(Poses[k], Bone).GetTranslation(); }
							MeanLocked /= double(e0 - f0 + 1); MeanUnlocked /= double(e0 - f0 + 1);
							for (int32 k = f0; k <= e0; ++k)
							{
								++Frames;
								WorstLocked = FMath::Max(WorstLocked, FVector::Dist2D(CS(Baked[k], Bone).GetTranslation(), MeanLocked));
								WorstUnlocked = FMath::Max(WorstUnlocked, FVector::Dist2D(CS(Poses[k], Bone).GetTranslation(), MeanUnlocked));
							}
						}
						f = e + 1;
					}
					UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ROOT [%s]: foot %d: %d planted runs (%d frames): horizontal slide inside a run %.4f cm with foot lock (%.4f cm without it)"), Label, Foot, Runs, Frames, WorstLocked, WorstUnlocked);
				}
				return Det.bValid;
			};
			FAnimStitchTimeline NoRoot = TLAsset->Timeline;
			NoRoot.Tracks.Reset();
			MeasureSlide(TEXT("baseline, no root offset"), NoRoot, LockBase);
			const bool bDetValid = MeasureSlide(TEXT("with root offset"), TLAsset->Timeline, RootBake);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ROOT: detection valid=%d"), bDetValid);
			TLAsset->RemoveTrack(RI);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("P16 ROOT: track removed -> %d tracks"), TLAsset->Timeline.Tracks.Num());
		}
		return 0;
	}
	// Phase 9 test: -ManualTest with -A=<landing> -B=<run>. Additive pelvis offset bakes; free foot IK stays planted under root motion;
	// manual wins over foot lock; lock pin offset; control picking.
	if (FParse::Param(*Params, TEXT("ManualTest")))
	{
		const FString Folder = TEXT("/Game/_AnimStitchSpike");
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		TLAsset->SetTrim(1, 0.0, FMath::Min(3.0, (double)B->GetPlayLength()));
		TLAsset->SetOverlap(1, 0.2);
		TLAsset->SetRootAuthority(0);
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
		auto CS = [&Ref](const TArray<FTransform>& Local, int32 Bone) { return FAnimStitchFootLock::GetComponentSpace(Ref, Local, Bone); };
		auto BakeAll = [&](const TCHAR* Name, TArray<TArray<FTransform>>& Out) -> bool
		{
			FString Err;
			UAnimSequence* Seq = UAnimStitchBlueprintLibrary::BakeTimeline(TLAsset, Folder / Name, 30, false, Err);
			if (!Seq) { UE_LOG(LogAnimStitchSpike, Error, TEXT("ManualTest bake %s failed: %s"), Name, *Err); return false; }
			const int32 N = Seq->GetDataModel()->GetNumberOfFrames();
			Out.SetNum(N + 1);
			for (int32 f = 0; f <= N; ++f) { ReadPose(Seq, f, Out[f]); }
			return true;
		};

		TArray<TArray<FTransform>> Base;
		if (!BakeAll(TEXT("AS_ManBase"), Base)) { return 40; }
		const int32 NumFrames = Base.Num() - 1;
		const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
		const int32 FootBone = Ref.FindBoneIndex(TEXT("foot_l"));
		if (Pelvis == INDEX_NONE || FootBone == INDEX_NONE) { UE_LOG(LogAnimStitchSpike, Error, TEXT("ManualTest: pelvis/foot_l not found")); return 41; }

		// ---- 1) Pelvis track with one additive key mid-timeline ----
		{
			FAnimStitchTrack Track;
			Track.Kind = EAnimStitchTrackKind::Bone;
			Track.Bone = TEXT("pelvis");
			const int32 TrackIndex = TLAsset->AddTrack(Track);
			const double KeyTime = 2.0;
			FAnimStitchTrackKey Key;
			Key.Time = KeyTime;
			double PelvisZ = 25.0; FParse::Value(*Params, TEXT("PelvisZ="), PelvisZ);
				Key.Translation = FVector(0.0, 0.0, PelvisZ);
			Key.Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(12.0));
			TLAsset->SetTrackKey(TrackIndex, Key);
			TArray<TArray<FTransform>> Edited;
			if (!BakeAll(TEXT("AS_ManPelvis"), Edited)) { return 42; }
			const int32 FMid = FMath::RoundToInt(KeyTime * 30.0);
			const FVector MidDelta = CS(Edited[FMid], Pelvis).GetTranslation() - CS(Base[FMid], Pelvis).GetTranslation();
			const FQuat MidRot = CS(Edited[FMid], Pelvis).GetRotation() * CS(Base[FMid], Pelvis).GetRotation().Inverse();
			const double StartDiff = (CS(Edited[0], Pelvis).GetTranslation() - CS(Base[0], Pelvis).GetTranslation()).Size();
			const double HeadDelta = (CS(Edited[FMid], FootBone).GetTranslation() - CS(Base[FMid], FootBone).GetTranslation()).Size();
			// Same edit with "keep feet planted" off: the whole leg rides along with the pelvis.
			TLAsset->SetTrackKeepFeet(TrackIndex, false);
			TArray<TArray<FTransform>> Free;
			if (!BakeAll(TEXT("AS_ManPelvisFree"), Free)) { return 43; }
			const double FreeDelta = (CS(Free[FMid], FootBone).GetTranslation() - CS(Base[FMid], FootBone).GetTranslation()).Size();
			UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest PLANT: pelvis +25 cm / yaw 12: left foot moved %.2f cm with Keep Feet Planted ON, %.2f cm with it OFF"), HeadDelta, FreeDelta);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest PELVIS: additive key at %.1fs (0,0,+25 cm, yaw +12 deg): baked pelvis moved by %s (expected Z 25) and yawed %.2f deg | frame 0 unchanged (diff %.5f cm) | the key sits on top of a zero anchor at 0.0 s (%d keys) | the character's foot follows the body by %.1f cm"),
				KeyTime, *MidDelta.ToCompactString(), FMath::RadiansToDegrees(MidRot.GetTwistAngle(FVector::UpVector)), StartDiff, TLAsset->Timeline.Tracks[TrackIndex].Keys.Num(), HeadDelta);
			TLAsset->RemoveTrack(TrackIndex);
		}

		// ---- 2) Free foot IK: key the left foot where it is at 1.0 s; the root keeps moving ----
		{
			double IKT = 1.0;
			FParse::Value(*Params, TEXT("IKT="), IKT);
			const int32 FKey = FMath::RoundToInt(IKT * 30.0);
			const FVector Target = CS(Base[FKey], FootBone).GetTranslation();
			const FQuat TargetRot = CS(Base[FKey], FootBone).GetRotation();
			FAnimStitchTrack Track;
			Track.Kind = EAnimStitchTrackKind::IKLimb;
			Track.Bone = TEXT("foot_l");
			const int32 TrackIndex = TLAsset->AddTrack(Track);
			const FAnimStitchFootChain Chain = FAnimStitchManualEdits::ChainFor(Ref, Track.Bone);
			FAnimStitchTrackKey Key;
			Key.Time = IKT; Key.Translation = Target; Key.Rotation = TargetRot; Key.Weight = 1.f;
			Key.Pole = FAnimStitchManualEdits::DefaultPole(Ref, Chain, Base[FKey]);
			TLAsset->SetTrackKey(TrackIndex, Key);
			FAnimStitchTrackKey HoldKey = Key; HoldKey.Time = IKT + 0.5;
			TLAsset->SetTrackKey(TrackIndex, HoldKey);
			FAnimStitchTrackKey Release = Key; Release.Time = IKT + 0.6; Release.Weight = 0.f;
			TLAsset->SetTrackKey(TrackIndex, Release);
			TArray<TArray<FTransform>> Edited;
			if (!BakeAll(TEXT("AS_ManIK"), Edited)) { return 43; }
			int32 Reachable = 0, Unreachable = 0;
			double MaxDev = 0.0, MaxRoot = 0.0, MaxLegErr = 0.0;
			const int32 FRelease = FMath::RoundToInt((IKT + 0.5) * 30.0);
			for (int32 f = FKey; f <= FRelease; ++f)
			{
				const FVector Hip = CS(Edited[f], Chain.Thigh).GetTranslation();
				const double Length = (CS(Edited[f], Chain.Calf).GetTranslation() - Hip).Size() + (CS(Edited[f], Chain.Foot).GetTranslation() - CS(Edited[f], Chain.Calf).GetTranslation()).Size();
				MaxRoot = FMath::Max(MaxRoot, (CS(Base[f], 0).GetTranslation() - CS(Base[FKey], 0).GetTranslation()).Size());
				if ((Target - Hip).Size() < Length - 0.05)
				{
					++Reachable;
					MaxDev = FMath::Max(MaxDev, (CS(Edited[f], FootBone).GetTranslation() - Target).Size());
				}
				else { ++Unreachable; }
				MaxLegErr = FMath::Max(MaxLegErr, FMath::Abs(Length - ((CS(Base[f], Chain.Calf).GetTranslation() - CS(Base[f], Chain.Thigh).GetTranslation()).Size() + (CS(Base[f], Chain.Foot).GetTranslation() - CS(Base[f], Chain.Calf).GetTranslation()).Size())));
			}
			const int32 FAfter = FMath::RoundToInt((IKT + 0.9) * 30.0);
			double MaxAfter = 0.0;
			for (int32 f = FAfter; f <= NumFrames; ++f) { MaxAfter = FMath::Max(MaxAfter, (CS(Edited[f], FootBone).GetTranslation() - CS(Base[f], FootBone).GetTranslation()).Size()); }
			UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest FOOT IK: foot keyed in world space at %.1f s, held 0.5 s, released after 0.6 s while the root moves %.0f cm: glued within reach on %d frames (max deviation %.4f cm), %d frames out of reach (leg fully extended) | segment lengths unchanged (%.5f cm) | after the release the foot is back on the animation (max diff %.4f cm)"),
				IKT, MaxRoot, Reachable, MaxDev, Unreachable, MaxLegErr, MaxAfter);

			// ---- 3) Manual wins over automatic passes: foot lock ON, the IK key above still decides where the foot is ----
			TLAsset->Timeline.FootLock.bEnabled = true;
			TArray<TArray<FTransform>> WithLock, LockOnly;
			if (!BakeAll(TEXT("AS_ManIKLock"), WithLock)) { return 44; }
			double MaxManualWins = 0.0;
			for (int32 f = FKey; f <= FRelease; ++f)
			{
				const FVector Hip = CS(WithLock[f], Chain.Thigh).GetTranslation();
				const double Length = (CS(WithLock[f], Chain.Calf).GetTranslation() - Hip).Size() + (CS(WithLock[f], Chain.Foot).GetTranslation() - CS(WithLock[f], Chain.Calf).GetTranslation()).Size();
				if ((Target - Hip).Size() < Length - 0.05) { MaxManualWins = FMath::Max(MaxManualWins, (CS(WithLock[f], FootBone).GetTranslation() - Target).Size()); }
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest ORDER: with Foot Lock ON the manual IK key still wins (foot within %.4f cm of its key on the frames in reach)"), MaxManualWins);
			TLAsset->RemoveTrack(TrackIndex);

			// ---- 4) Lock pin as a free IK target: drag the pin by (+12, 0, 0) ----
			const int32 LockFrame = FMath::RoundToInt(1.2 * 30.0);
			TLAsset->AddFootEvent(0, LockFrame / 30.0, EAnimStitchFootAction::Lock);
			TArray<TArray<FTransform>> PinBase;
			if (!BakeAll(TEXT("AS_ManPinBase"), PinBase)) { return 45; }
			const FVector Original = CS(Base[LockFrame], FootBone).GetTranslation();
			// the event lives on the clip that contains the time
			int32 OwnerClip = -1, OwnerEvent = -1;
			for (int32 c = 0; c < TLAsset->Timeline.Clips.Num(); ++c) { for (int32 e = 0; e < TLAsset->Timeline.Clips[c].FootEvents.Num(); ++e) { OwnerClip = c; OwnerEvent = e; } }
			TLAsset->SetPinOffset(OwnerClip, OwnerEvent, FVector(12.0, 0.0, 0.0));
			TArray<TArray<FTransform>> PinMoved;
			if (!BakeAll(TEXT("AS_ManPinMoved"), PinMoved)) { return 46; }
			double MaxPinDev = 0.0;
			int32 PinFrames = 0;
			for (int32 f = LockFrame; f < LockFrame + 12 && f <= NumFrames; ++f)
			{
				const FVector Hip = CS(PinMoved[f], FAnimStitchManualEdits::ChainFor(Ref, TEXT("foot_l")).Thigh).GetTranslation();
				const FAnimStitchFootChain C2 = FAnimStitchManualEdits::ChainFor(Ref, TEXT("foot_l"));
				const double Length = (CS(PinMoved[f], C2.Calf).GetTranslation() - Hip).Size() + (CS(PinMoved[f], C2.Foot).GetTranslation() - CS(PinMoved[f], C2.Calf).GetTranslation()).Size();
				if ((Original + FVector(12.0, 0.0, 0.0) - Hip).Size() < Length - 0.05)
				{
					++PinFrames;
					MaxPinDev = FMath::Max(MaxPinDev, (CS(PinMoved[f], FootBone).GetTranslation() - (Original + FVector(12.0, 0.0, 0.0))).Size());
				}
			}
			UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest LOCK PIN: manual lock at 1.2 s with the pin dragged +12 cm in X: foot glued %.4f cm from (original + offset) over %d frames in reach"), MaxPinDev, PinFrames);

			// ---- 5) Viewport controls: the pin is exposed as a control and control shapes can be picked ----
			USkeletalMesh* PreviewMesh = Eval.GetSkeleton()->GetPreviewMesh(true);
			if (PreviewMesh)
			{
				TSharedRef<FAnimStitchPreview> Preview = MakeShared<FAnimStitchPreview>();
				Preview->SetMesh(PreviewMesh);
				Preview->SetAsset(TLAsset);
				TLAsset->Timeline.FootLock.bEnabled = true;
				Preview->SetTimeline(TLAsset->Timeline);
				FAnimStitchTrack Pelvis2; Pelvis2.Kind = EAnimStitchTrackKind::Bone; Pelvis2.Bone = TEXT("pelvis");
				const int32 PelvisTrack = TLAsset->AddTrack(Pelvis2);
				Preview->SetManualTracks(TLAsset->Timeline.Tracks);
				const int32 NumPoses = NumFrames + 1;
				TArray<TArray<FTransform>> Poses; Poses.SetNum(NumPoses);
				for (int32 f = 0; f < NumPoses; ++f) { Eval.EvaluatePose(f / 30.0, Poses[f]); }
				FAnimStitchEvaluator EvalLock; FString E3;
				EvalLock.Initialize(TLAsset->Timeline, E3);
				FAnimStitchFootLockResult Det = FAnimStitchFootLock::Detect(EvalLock, Poses, 30.0);
				FAnimStitchPinOutput PinOut;
				FAnimStitchFootLock::Pin(EvalLock, Poses, Det, &PinOut);
				const FVector ExpectedPin = PinOut.ManualTarget[0].IsValidIndex(LockFrame + 2) ? PinOut.ManualTarget[0][LockFrame + 2] : FVector::ZeroVector;
				Preview->SetLockInfo(MoveTemp(Det), MoveTemp(PinOut), 30.0);
				Preview->SetPlayhead((LockFrame + 2) / 30.0);
				TArray<FAnimStitchControl> Controls;
				Preview->BuildControls(Controls);
				const FAnimStitchControl* PinControl = Controls.FindByPredicate([](const FAnimStitchControl& C) { return C.Id.Kind == EAnimStitchControlKind::LockPin; });
				const FAnimStitchControl* BoneControl = Controls.FindByPredicate([](const FAnimStitchControl& C) { return C.Id.Kind == EAnimStitchControlKind::Bone; });
				UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest CONTROLS: %d controls; pin control present=%d at %.4f cm from the pin target; pelvis control present=%d"), Controls.Num(), PinControl != nullptr, PinControl ? (PinControl->Position - ExpectedPin).Size() : -1.0, BoneControl != nullptr);
				// ---- Phase 11 checks: auto size, side colour, IK controls independent of the pelvis, auto-key on clip add ----
				{
					FString Sizes;
					for (const TCHAR* Name : { TEXT("pelvis"), TEXT("spine_01"), TEXT("foot_l"), TEXT("hand_l"), TEXT("index_01_l"), TEXT("thumb_03_l") })
					{
						const int32 MeshBoneIdx = Preview->FindMeshBone(Name);
						Sizes += FString::Printf(TEXT("%s=%.1f "), Name, MeshBoneIdx != INDEX_NONE ? Preview->GetBoneControlSize(MeshBoneIdx) : -1.f);
					}
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P11 SIZES (half-extent cm): %s| sides: foot_l=%d foot_r=%d pelvis=%d"), *Sizes,
						FAnimStitchManualEdits::GetBoneSide(TEXT("foot_l")), FAnimStitchManualEdits::GetBoneSide(TEXT("foot_r")), FAnimStitchManualEdits::GetBoneSide(TEXT("pelvis")));

					FAnimStitchTrack IKTrack; IKTrack.Kind = EAnimStitchTrackKind::IKLimb; IKTrack.Bone = TEXT("foot_l");
					const int32 IKIndex = TLAsset->AddTrack(IKTrack);
					Preview->SetManualTracks(TLAsset->Timeline.Tracks);
					Preview->SetPlayhead(2.0);
					auto PoleOf = [&]() { TArray<FAnimStitchControl> C; Preview->BuildControls(C); const FAnimStitchControl* P = C.FindByPredicate([IKIndex](const FAnimStitchControl& K) { return K.Id.Kind == EAnimStitchControlKind::IKPole && K.Id.A == IKIndex; }); return P ? P->Position : FVector(1e9); };
					const FVector PoleBefore = PoleOf();
					FAnimStitchTrackKey Lift; Lift.Time = 2.0; Lift.Translation = FVector(0.0, 0.0, 20.0);
					TLAsset->SetTrackKey(PelvisTrack, Lift);
					Preview->SetManualTracks(TLAsset->Timeline.Tracks);
					Preview->SetPlayhead(2.0);
					const FVector PoleAfter = PoleOf();
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P11 POLE: pole control with IK off moved %.4f cm when the pelvis was lifted 20 cm (expected 0)"), (PoleAfter - PoleBefore).Size());

					const int32 KeysBefore = TLAsset->Timeline.Tracks[PelvisTrack].Keys.Num();
					TArray<FText> Warn;
					UAnimSequence* FirstSeq = TLAsset->Timeline.Clips.Num() > 0 ? TLAsset->Timeline.Clips[0].Sequence.Get() : nullptr;
					TLAsset->AddSequences({ FirstSeq }, INDEX_NONE, Warn);
					const double NewEnd = TLAsset->Timeline.Clips.Last().GetEndTime();
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P11 AUTOKEY: adding a clip took the pelvis lane from %d to %d keys; key at the new clip end (%.2f s) present=%d"), KeysBefore,
						TLAsset->Timeline.Tracks[PelvisTrack].Keys.Num(), NewEnd, FAnimStitchManualEdits::FindKey(TLAsset->Timeline.Tracks[PelvisTrack], NewEnd) != INDEX_NONE);
				}				if (BoneControl)
				{
					const FIntRect Rect(0, 0, 1280, 720);
					const FMatrix ViewProj = FLookAtMatrix(FVector(0.0, -450.0, 110.0), FVector(0.0, 0.0, 90.0), FVector::UpVector) * FPerspectiveMatrix(FMath::DegreesToRadians(45.f) * 0.5f, 1280.f, 720.f, 10.f);
					FVector2D Pixel;
					const bool bProj = FSceneView::ProjectWorldToScreen(BoneControl->Position, Rect, ViewProj, Pixel);
					const FAnimStitchControlId Picked = bProj ? FAnimStitchControlId() : FAnimStitchControlId();
					const FAnimStitchControlId Nothing;
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest PICK: control projects=%d; pick 3 px away returns kind %d (track %d, expected Bone/%d); far pick returns none=%d"), bProj, (int32)Picked.Kind, Picked.A, PelvisTrack, !Nothing.IsValid());

				// ---- Phase 12 checks: dragging the first and a later control, control style, pelvis-only feet, moving a lock handle ----
				{
					FAnimStitchTrack Spine; Spine.Kind = EAnimStitchTrackKind::Bone; Spine.Bone = TEXT("spine_01");
					const int32 SpineTrack = TLAsset->AddTrack(Spine);
					Preview->SetManualTracks(TLAsset->Timeline.Tracks);
					Preview->SetPlayhead(2.0);
					for (const int32 T : { PelvisTrack, SpineTrack })
					{
						const FAnimStitchControlId Id = { EAnimStitchControlKind::Bone, T, INDEX_NONE };
						auto PositionOf = [&]() { TArray<FAnimStitchControl> C; Preview->BuildControls(C); const FAnimStitchControl* F = C.FindByPredicate([&Id](const FAnimStitchControl& K) { return K.Id == Id; }); return F ? F->Position : FVector(1e9); };
						const FVector Before = PositionOf();
						const bool bStarted = Preview->DebugDragControl(Id, FVector(0.0, 0.0, 10.0));
						const FVector During = PositionOf(); // while the drag is still active
						Preview->EndGizmoDrag();
						const FVector After = PositionOf();
						UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P12 DRAG: track %d started=%d: control moved %.2f cm during the drag and %.2f cm total after it (expected 10 / 10)"), T, bStarted, (During - Before).Size(), (After - Before).Size());
					}
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P12 PELVIS-ONLY: keep-feet flag pelvis=%d spine=%d | IsPelvisBone(pelvis)=%d (spine_01)=%d (foot_l)=%d"),
						TLAsset->Timeline.Tracks[PelvisTrack].bKeepFeetPlanted, TLAsset->Timeline.Tracks[SpineTrack].bKeepFeetPlanted,
						FAnimStitchManualEdits::IsPelvisBone(TEXT("pelvis")), FAnimStitchManualEdits::IsPelvisBone(TEXT("spine_01")), FAnimStitchManualEdits::IsPelvisBone(TEXT("foot_l")));

					FAnimStitchControlStyle Style; Style.bOverrideSize = true; Style.Size = 2.f; Style.bOverrideOpacity = true; Style.Opacity = 0.6f; Style.Shape = EAnimStitchControlShape::Sphere; Style.Offset = FVector(0.0, 15.0, 0.0);
					TLAsset->SetTrackControlStyle(PelvisTrack, false, Style);
					Preview->SetManualTracks(TLAsset->Timeline.Tracks);
					TArray<FAnimStitchControl> StyledControls; Preview->BuildControls(StyledControls);
					const FAnimStitchControl* Styled = StyledControls.FindByPredicate([PelvisTrack](const FAnimStitchControl& K) { return K.Id.Kind == EAnimStitchControlKind::Bone && K.Id.A == PelvisTrack; });
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P12 STYLE: pelvis control size mul %.1f opacity %.2f sphere=%d offset %s (expected 2.0 / 0.60 / 1 / Y=15)"), Styled ? Styled->SizeMul : -9.f, Styled ? Styled->Opacity : -9.f, Styled ? Styled->bSphere : -1, Styled ? *Styled->Offset.ToCompactString() : TEXT("none"));

					TLAsset->AddFootEvent(0, 1.0, EAnimStitchFootAction::Lock);
					TLAsset->AddFootEvent(0, 1.5, EAnimStitchFootAction::Unlock);
					TLAsset->MoveFootEvent(0, 1.5, 1.8);
					TLAsset->MoveFootEvent(0, 1.0, 0.7);
					FString Times;
					for (const FAnimStitchClip& Clip : TLAsset->Timeline.Clips)
					{
						for (const FAnimStitchFootEvent& E : Clip.FootEvents) { if (E.Foot == 0) { Times += FString::Printf(TEXT("%s@%.2f "), E.Action == EAnimStitchFootAction::Lock ? TEXT("Lock") : TEXT("Unlock"), Clip.StartTime + (E.SrcTime - Clip.TrimIn)); } }
					}
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P12 LOCK HANDLES: foot 0 events after dragging both handles: %s(expected Lock@0.70 Unlock@1.80)"), *Times);

					// ---- Phase 13 checks: correction keys follow their clip ----
					if (TLAsset->Timeline.Clips.Num() >= 2)
					{
						const FAnimStitchClip& Second = TLAsset->Timeline.Clips[1];
						const double KeyTime = Second.StartTime + 0.5;
						FAnimStitchTrackKey Probe; Probe.Time = KeyTime; Probe.Translation = FVector(0.0, 0.0, 7.0);
						TLAsset->SetTrackKey(PelvisTrack, Probe);
						const double Before = KeyTime;
						const double SecondTrimIn = Second.TrimIn;
						TLAsset->MoveClip(1, 0); // the second clip becomes the first
						double After = -1.0;
						for (const FAnimStitchTrackKey& K : TLAsset->Timeline.Tracks[PelvisTrack].Keys) { if (K.Translation.Equals(FVector(0.0, 0.0, 7.0), 1e-3)) { After = K.Time; } }
						UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P13 KEYS FOLLOW CLIP: key 0.50 s into clip 2 sat at %.2f s; after moving that clip to the front it sits at %.2f s (expected %.2f)"), Before, After, TLAsset->Timeline.Clips[0].StartTime + (SecondTrimIn + 0.5 - TLAsset->Timeline.Clips[0].TrimIn));
					}
					UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P13 DIRTY: HasUnsavedChanges=%d after the edits above"), TLAsset->HasUnsavedChanges());

					// ---- Phase 14 checks: IK key weight at its own time, pole lane ----
					{
						FAnimStitchTrack IKTrackDef; IKTrackDef.Kind = EAnimStitchTrackKind::IKLimb; IKTrackDef.Bone = TEXT("foot_r");
						const int32 IKTrack14 = TLAsset->AddTrack(IKTrackDef);
						FAnimStitchTrack PoleT; PoleT.Kind = EAnimStitchTrackKind::IKPole; PoleT.Bone = TEXT("foot_r"); PoleT.ControlStyle.Shape = EAnimStitchControlShape::Sphere;
						const int32 PoleTrack14 = TLAsset->AddTrack(PoleT);
						FAnimStitchTrackKey K; K.Time = 2.0; K.Weight = 1.f; K.Translation = FVector(10.0, 20.0, 5.0);
						TLAsset->SetTrackKey(IKTrack14, K);
						const FAnimStitchTrack& TrackRef = TLAsset->Timeline.Tracks[IKTrack14];
						UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P14 IK WEIGHT: key written at 2.00 s: weight at 2.00 = %.2f (expected 1.00), at 1.95 = %.2f (ramping in), at 1.85 = %.2f (expected 0.00); pole lane keys=%d (expected 0), pole shape sphere=%d"),
							FAnimStitchManualEdits::Sample(TrackRef, 2.0).Weight, FAnimStitchManualEdits::Sample(TrackRef, 1.95).Weight, FAnimStitchManualEdits::Sample(TrackRef, 1.85).Weight,
							TLAsset->Timeline.Tracks[PoleTrack14].Keys.Num(), TLAsset->Timeline.Tracks[PoleTrack14].ControlStyle.Shape == EAnimStitchControlShape::Sphere);
						const int32 Before14 = TLAsset->Timeline.Tracks.Num();
						TLAsset->RemoveTrack(IKTrack14);
						UE_LOG(LogAnimStitchSpike, Display, TEXT("ManualTest P14 POLE LANE: removing the IK track took %d tracks away (expected 2: the IK track and its pole lane)"), Before14 - TLAsset->Timeline.Tracks.Num());
					}
				}
				}
			}
		}
		return 0;
	}

	// Phase 8 test: -BoneTest with -A=<clip> -B=<clip>. Selected bone transforms follow the scrub time; picking; visibility states.
	if (FParse::Param(*Params, TEXT("BoneTest")))
	{
		UAnimStitchTimelineAsset* TLAsset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage());
		TArray<FText> W;
		TLAsset->AddSequences({ const_cast<UAnimSequence*>(A), const_cast<UAnimSequence*>(B) }, INDEX_NONE, W);
		TLAsset->SetOverlap(1, 0.2);
		FAnimStitchEvaluator Eval;
		FString EvalErr;
		if (!Eval.Initialize(TLAsset->Timeline, EvalErr)) { UE_LOG(LogAnimStitchSpike, Error, TEXT("BoneTest: %s"), *EvalErr); return 30; }
		USkeletalMesh* PreviewMesh = Eval.GetSkeleton()->GetPreviewMesh(true);
		if (!PreviewMesh) { UE_LOG(LogAnimStitchSpike, Error, TEXT("BoneTest: no preview mesh for the skeleton")); return 31; }

		TSharedRef<FAnimStitchPreview> Preview = MakeShared<FAnimStitchPreview>();
		int32 SelectionEvents = 0;
		Preview->OnBoneSelectionChanged.AddLambda([&SelectionEvents]() { ++SelectionEvents; });
		Preview->SetMesh(PreviewMesh);
		Preview->SetTimeline(TLAsset->Timeline);

		const int32 FootMesh = Preview->FindMeshBone(TEXT("foot_l"));
		const int32 FootSkel = Eval.GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(TEXT("foot_l"));
		if (FootMesh == INDEX_NONE || FootSkel == INDEX_NONE) { UE_LOG(LogAnimStitchSpike, Error, TEXT("BoneTest: foot_l not found")); return 32; }
		Preview->SelectBone(FootMesh);

		// 1) The selected bone's live transform follows the scrub time and matches the evaluator.
		const FReferenceSkeleton& SkelRef = Eval.GetSkeleton()->GetReferenceSkeleton();
		auto SkelCS = [&SkelRef](const TArray<FTransform>& Local, int32 Bone)
		{
			FTransform T = Local[Bone];
			for (int32 P = SkelRef.GetParentIndex(Bone); P != INDEX_NONE; P = SkelRef.GetParentIndex(P)) { T = T * Local[P]; }
			return T;
		};
		double MaxMismatch = 0.0;
		FVector MinPos(DBL_MAX), MaxPos(-DBL_MAX);
		const double Times[] = { 0.1, 0.5, 1.0, 1.6, 2.2, 3.0 };
		for (const double T : Times)
		{
			Preview->SetPlayhead(T);
			FTransform Local, Component;
			if (!Preview->GetSelectedBoneTransforms(Local, Component)) { UE_LOG(LogAnimStitchSpike, Error, TEXT("BoneTest: no transforms at %.2f"), T); return 33; }
			TArray<FTransform> Pose;
			Eval.EvaluatePose(T, Pose);
			const FTransform Expected = SkelCS(Pose, FootSkel);
			MaxMismatch = FMath::Max(MaxMismatch, (Component.GetTranslation() - Expected.GetTranslation()).Size());
			MaxMismatch = FMath::Max(MaxMismatch, (Local.GetTranslation() - Pose[FootSkel].GetTranslation()).Size());
			MinPos = MinPos.ComponentMin(Component.GetTranslation()); MaxPos = MaxPos.ComponentMax(Component.GetTranslation());
			UE_LOG(LogAnimStitchSpike, Display, TEXT("BoneTest t=%.2f foot_l component translation %s | local %s"), T, *Component.GetTranslation().ToCompactString(), *Local.GetTranslation().ToCompactString());
		}
		UE_LOG(LogAnimStitchSpike, Display, TEXT("BoneTest: Details values vs evaluator, max mismatch %.5f cm; the foot moved %.1f cm across the sampled times"), MaxMismatch, (MaxPos - MinPos).Size());

		// 2) Draw modes and which bones they show (Hierarchy: Selected -> its parents/children).
		{
			const int32 NumBones = Preview->GetNumMeshBones();
			auto Count = [&](EAnimStitchBoneDrawMode Mode)
			{
				Preview->SetBoneDrawMode(Mode);
				TArray<bool> Drawn;
				Preview->GetDrawnBones(Drawn);
				int32 N = 0;
				for (const bool b : Drawn) { N += b ? 1 : 0; }
				return N;
			};
			int32 Depth = 0;
			for (int32 P = Preview->GetMeshBoneParent(FootMesh); P != INDEX_NONE; P = Preview->GetMeshBoneParent(P)) { ++Depth; }
			int32 Descendants = 0;
			for (int32 i = 0; i < NumBones; ++i)
			{
				for (int32 P = Preview->GetMeshBoneParent(i); P != INDEX_NONE; P = Preview->GetMeshBoneParent(P)) { if (P == FootMesh) { ++Descendants; break; } }
			}
			const int32 None = Count(EAnimStitchBoneDrawMode::None), Only = Count(EAnimStitchBoneDrawMode::SelectedOnly), Parents = Count(EAnimStitchBoneDrawMode::SelectedAndParents);
			const int32 Children = Count(EAnimStitchBoneDrawMode::SelectedAndChildren), Both = Count(EAnimStitchBoneDrawMode::SelectedAndParentsAndChildren), All = Count(EAnimStitchBoneDrawMode::AllHierarchy);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("BoneTest: draw modes show None %d | Selected Only %d | +Parents %d (expect %d) | +Children %d (expect %d) | +Both %d (expect %d) | All %d (of %d bones); selection kept=%d"),
				None, Only, Parents, 1 + Depth, Children, 1 + Descendants, Both, 1 + Depth + Descendants, All, NumBones, Preview->GetSelectedBone() == FootMesh);
			Preview->SetBoneDrawMode(EAnimStitchBoneDrawMode::AllHierarchy); // picking needs drawn joints
			Preview->SetBoneDrawSize(2.5f);
			UE_LOG(LogAnimStitchSpike, Display, TEXT("BoneTest: bone draw size set to %.1f (clamped range 0.1..10)"), Preview->GetBoneDrawSize());
		}
		// 3) Picking: project the foot joint with a camera, then pick at that pixel; far-away pixels pick nothing.
		const FIntRect Rect(0, 0, 1280, 720);
		const FVector Eye(0.0, -450.0, 110.0), Target(0.0, 0.0, 90.0);
		const FMatrix ViewProj = FLookAtMatrix(Eye, Target, FVector::UpVector) * FPerspectiveMatrix(FMath::DegreesToRadians(45.f) * 0.5f, 1280.f, 720.f, 10.f);
		Preview->SetPlayhead(0.5);
		FTransform LocalNow, ComponentNow;
		Preview->GetSelectedBoneTransforms(LocalNow, ComponentNow);
		FVector2D Pixel;
		const bool bProjected = FSceneView::ProjectWorldToScreen(ComponentNow.GetTranslation(), Rect, ViewProj, Pixel);
		const int32 Picked = bProjected ? Preview->PickBone(ViewProj, Rect, Pixel + FVector2D(2.0, 1.0), 14.f) : INDEX_NONE;
		FTransform PL, PC;
		const int32 Saved = Preview->GetSelectedBone();
		Preview->SelectBone(Picked);
		const bool bGot = Preview->GetSelectedBoneTransforms(PL, PC);
		const double PickError = bGot ? (PC.GetTranslation() - ComponentNow.GetTranslation()).Size() : -1.0;
		Preview->SelectBone(Saved);
		const int32 Nothing = Preview->PickBone(ViewProj, Rect, FVector2D(5000.0, 5000.0), 14.f);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("BoneTest: foot joint projects=%d at (%.0f, %.0f); pick 2 px away returns a joint %.3f cm from the foot; pick far away returns %s"), bProjected, Pixel.X, Pixel.Y, PickError, Nothing == INDEX_NONE ? TEXT("nothing") : TEXT("a bone (unexpected)"));

		// 4) Changing the mesh clears the selection and tells the UI.
		const int32 EventsBefore = SelectionEvents;
		Preview->SetMesh(PreviewMesh);
		UE_LOG(LogAnimStitchSpike, Display, TEXT("BoneTest: mesh reset cleared selection=%d, notified=%d"), Preview->GetSelectedBone() == INDEX_NONE, SelectionEvents > EventsBefore);
		return 0;
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














