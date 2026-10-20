#include "AnimStitchSpikeCommandlet.h"
#include "AnimStitchBaker.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchBatch.h"
#include "AnimStitchBlueprintLibrary.h"
#include "AnimStitchFootLock.h"
#include "AnimStitchManualEdits.h"
#include "AnimStitchPreview.h"
#include "Engine/SkeletalMesh.h"
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














