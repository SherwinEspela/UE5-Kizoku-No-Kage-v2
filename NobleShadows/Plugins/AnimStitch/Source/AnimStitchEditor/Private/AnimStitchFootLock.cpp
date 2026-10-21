#include "AnimStitchFootLock.h"
#include "AnimStitchSettings.h"
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace
{
	/** Component-space transform of Bone from skeleton-local poses. */
	FTransform ComponentSpace(const FReferenceSkeleton& Ref, const TArray<FTransform>& Local, int32 Bone)
	{
		FTransform Result = Local[Bone];
		for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
		{
			Result = Result * Local[Parent];
		}
		return Result;
	}

	bool IsSideMatch(const FString& Lower, bool bLeft)
	{
		if (bLeft)
		{
			return Lower.Contains(TEXT("left")) || Lower.EndsWith(TEXT("_l")) || Lower.EndsWith(TEXT(".l")) || Lower.Contains(TEXT("_l_")) || Lower.Contains(TEXT(" l ")) || Lower.StartsWith(TEXT("l_"));
		}
		return Lower.Contains(TEXT("right")) || Lower.EndsWith(TEXT("_r")) || Lower.EndsWith(TEXT(".r")) || Lower.Contains(TEXT("_r_")) || Lower.Contains(TEXT(" r ")) || Lower.StartsWith(TEXT("r_"));
	}
}

FName FAnimStitchFootLock::GuessFootBone(const FReferenceSkeleton& Ref, bool bLeft)
{
	static const TCHAR* Excluded[] = { TEXT("ik"), TEXT("twist"), TEXT("roll"), TEXT("corrective"), TEXT("toe"), TEXT("ball"), TEXT("end"), TEXT("helper"), TEXT("pole") };
	FName Best;
	int32 BestScore = -1;
	for (int32 i = 0; i < Ref.GetNum(); ++i)
	{
		const FString Lower = Ref.GetBoneName(i).ToString().ToLower();
		if (!Lower.Contains(TEXT("foot")) || !IsSideMatch(Lower, bLeft))
		{
			continue;
		}
		bool bBad = false;
		for (const TCHAR* Word : Excluded)
		{
			// "ik" must be a whole token (e.g. "ik_foot_l"), not part of another word.
			bBad |= (FCString::Strcmp(Word, TEXT("ik")) == 0) ? (Lower.StartsWith(TEXT("ik")) || Lower.Contains(TEXT("_ik")) || Lower.Contains(TEXT("ik_")) || Lower.EndsWith(TEXT("ik"))) : Lower.Contains(Word);
		}
		if (bBad)
		{
			continue;
		}
		const int32 Score = 1000 - Lower.Len(); // shortest plausible name wins
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Ref.GetBoneName(i);
		}
	}
	return Best;
}

void FAnimStitchFootLock::ResolveFootBones(const USkeleton* Skeleton, const FAnimStitchFootLockSettings& Settings, FName& OutLeft, FName& OutRight)
{
	OutLeft = Settings.LeftFootBone;
	OutRight = Settings.RightFootBone;
	if (!Skeleton)
	{
		return;
	}
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	const FAnimStitchFootBones* Remembered = GetDefault<UAnimStitchSettings>()->FootBonesBySkeleton.Find(Skeleton->GetPathName());
	if (OutLeft.IsNone())
	{
		OutLeft = (Remembered && Ref.FindBoneIndex(Remembered->Left) != INDEX_NONE) ? Remembered->Left : GuessFootBone(Ref, true);
	}
	if (OutRight.IsNone())
	{
		OutRight = (Remembered && Ref.FindBoneIndex(Remembered->Right) != INDEX_NONE) ? Remembered->Right : GuessFootBone(Ref, false);
	}
}

FAnimStitchFootChain FAnimStitchFootLock::BuildChain(const FReferenceSkeleton& Ref, FName FootBone)
{
	FAnimStitchFootChain Chain;
	Chain.Foot = Ref.FindBoneIndex(FootBone);
	if (Chain.Foot == INDEX_NONE)
	{
		return Chain;
	}
	Chain.Calf = Ref.GetParentIndex(Chain.Foot);
	Chain.Thigh = Chain.Calf != INDEX_NONE ? Ref.GetParentIndex(Chain.Calf) : INDEX_NONE;
	Chain.ThighParent = Chain.Thigh != INDEX_NONE ? Ref.GetParentIndex(Chain.Thigh) : INDEX_NONE;
	return Chain;
}

FAnimStitchFootLockResult FAnimStitchFootLock::Detect(const FAnimStitchEvaluator& Evaluator, const TArray<TArray<FTransform>>& Poses, double Fps)
{
	FAnimStitchFootLockResult Result;
	Result.Fps = Fps;
	const FAnimStitchTimeline& Timeline = Evaluator.GetTimeline();
	const FAnimStitchFootLockSettings& S = Timeline.FootLock;
	const USkeleton* Skeleton = Evaluator.GetSkeleton();
	if (!Skeleton || Poses.Num() < 2 || Fps <= 0.0)
	{
		Result.Problem = LOCTEXT("FootNoData", "Foot lock has nothing to analyse.");
		return Result;
	}
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();

	FName Bones[2];
	ResolveFootBones(Skeleton, S, Bones[0], Bones[1]);
	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		Result.FootBones[Foot] = Bones[Foot];
		Result.Chains[Foot] = BuildChain(Ref, Bones[Foot]);
		if (!Result.Chains[Foot].IsValid())
		{
			Result.Problem = FText::Format(LOCTEXT("FootNoChain", "Foot lock: the {0} foot bone '{1}' was not found or has no thigh/calf above it. Pick the foot bones in the Bake window."),
				Foot == 0 ? LOCTEXT("Left", "left") : LOCTEXT("Right", "right"), FText::FromName(Bones[Foot]));
			return Result;
		}
	}

	if (S.bAutoGroundHeight)
	{
		const TArray<FTransform>& RefPose = Ref.GetRefBonePose();
		Result.GroundZ = FMath::Min(ComponentSpace(Ref, RefPose, Result.Chains[0].Foot).GetTranslation().Z, ComponentSpace(Ref, RefPose, Result.Chains[1].Foot).GetTranslation().Z);
	}
	else
	{
		Result.GroundZ = S.GroundHeight;
	}

	const int32 NumFrames = Poses.Num();
	const double Dt = 1.0 / Fps;
	const int32 MinFrames = FMath::Max(1, FMath::CeilToInt(S.MinPlantDuration * Fps));

	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		TArray<FVector> Position;
		Position.SetNum(NumFrames);
		for (int32 f = 0; f < NumFrames; ++f)
		{
			Position[f] = ComponentSpace(Ref, Poses[f], Result.Chains[Foot].Foot).GetTranslation();
		}

		TArray<uint8> Planted;
		Planted.Init(0, NumFrames);
		for (int32 f = 0; f < NumFrames; ++f)
		{
			const int32 Prev = FMath::Max(f - 1, 0);
			const int32 Next = FMath::Min(f + 1, NumFrames - 1);
			const double Speed = (Position[Next] - Position[Prev]).Size() / ((Next - Prev) * Dt);
			Planted[f] = (Position[f].Z - Result.GroundZ <= S.HeightThreshold) && Speed <= S.VelocityThreshold ? 1 : 0;
		}

		// Drop plants shorter than the minimum duration.
		for (int32 f = 0; f < NumFrames;)
		{
			if (!Planted[f]) { ++f; continue; }
			int32 End = f;
			while (End + 1 < NumFrames && Planted[End + 1]) { ++End; }
			if (End - f + 1 < MinFrames)
			{
				for (int32 k = f; k <= End; ++k) { Planted[k] = 0; }
			}
			f = End + 1;
		}

		// Manual events (stored per clip in source time) mapped onto this timeline. Persistent model: before the foot's
		// first event automatic detection applies; from then on each event decides until the next one.
		struct FMappedEvent { int32 Frame; double Time; EAnimStitchFootAction Action; FVector PinOffset; FIntPoint Owner; };
		TArray<FMappedEvent> Events;
		for (int32 ClipIndex = 0; ClipIndex < Timeline.Clips.Num(); ++ClipIndex)
		{
			const FAnimStitchClip& Clip = Timeline.Clips[ClipIndex];
			for (int32 EventIndex = 0; EventIndex < Clip.FootEvents.Num(); ++EventIndex)
			{
				const FAnimStitchFootEvent& Event = Clip.FootEvents[EventIndex];
				if (Event.Foot != Foot) { continue; }
const double Time = Clip.TimelineTimeAt(Event.SrcTime);
				FMappedEvent Mapped;
				Mapped.Frame = FMath::Max<int32>(0, FMath::RoundToInt32(Time * Fps));
				Mapped.Time = Time;
				Mapped.Action = Event.Action;
				Mapped.PinOffset = Event.PinOffset;
				Mapped.Owner = FIntPoint(ClipIndex, EventIndex);
				Events.Add(Mapped);
			}
		}
		Events.StableSort([](const FMappedEvent& A, const FMappedEvent& B) { return A.Time < B.Time; });
		for (const FMappedEvent& E : Events)
		{
			FAnimStitchFootLockResult::FEventMark Mark;
			Mark.Time = E.Time;
			Mark.Action = E.Action;
			Result.Events[Foot].Add(Mark);
		}

		TArray<uint8>& Mask = Result.Mask[Foot];
		Mask.Init(0, NumFrames);
		Result.PinOffset[Foot].Init(FVector::ZeroVector, NumFrames);
		Result.PinEvent[Foot].Init(FIntPoint(-1, -1), NumFrames);
		FVector CurrentPinOffset = FVector::ZeroVector;
		FIntPoint CurrentPinOwner(-1, -1);
		int32 NextEvent = 0;
		bool bManual = false;                                    // any event seen yet?
		EAnimStitchFootAction State = EAnimStitchFootAction::Auto;
		for (int32 f = 0; f < NumFrames; ++f)
		{
			while (NextEvent < Events.Num() && Events[NextEvent].Frame <= f)
			{
				State = Events[NextEvent].Action;
				CurrentPinOffset = Events[NextEvent].PinOffset;
				CurrentPinOwner = Events[NextEvent].Owner;
				bManual = true;
				++NextEvent;
			}
			if (!bManual || State == EAnimStitchFootAction::Auto)
			{
				Mask[f] = Planted[f] ? AnimStitchPlant::Auto : AnimStitchPlant::None;
			}
			else if (State == EAnimStitchFootAction::Lock)
			{
				Mask[f] = AnimStitchPlant::ManualPin;
				Result.PinOffset[Foot][f] = CurrentPinOffset;
				Result.PinEvent[Foot][f] = CurrentPinOwner;
			}
			else
			{
				Mask[f] = Planted[f] ? AnimStitchPlant::Suppressed : AnimStitchPlant::None; // released by hand
			}
		}
	}
	Result.bValid = true;
	return Result;
}

namespace
{
	/**
	 * Moves the ankle of one leg to Desired using analytic two-bone IK, keeping the knee on the side it already bends to
	 * and the foot's world orientation. Leg segment lengths are preserved; if Desired is out of reach the leg is fully
	 * extended towards it (the hips are never moved).
	 */
	void SolveLeg(const FReferenceSkeleton& Ref, TArray<FTransform>& Local, const FAnimStitchFootChain& Chain, const FVector& Desired,
		const FVector* PoleTarget = nullptr, double PoleWeight = 0.0, const FQuat* EffectorRotation = nullptr)
	{
		const FTransform ParentCS = Chain.ThighParent != INDEX_NONE ? ComponentSpace(Ref, Local, Chain.ThighParent) : FTransform::Identity;
		const FTransform ThighCS = Local[Chain.Thigh] * ParentCS;
		const FTransform CalfCS = Local[Chain.Calf] * ThighCS;
		const FTransform FootCS = Local[Chain.Foot] * CalfCS;

		const FVector H = ThighCS.GetTranslation();
		const FVector K = CalfCS.GetTranslation();
		const FVector A = FootCS.GetTranslation();
		const double L1 = (K - H).Size();
		const double L2 = (A - K).Size();
		const FVector ToTarget = Desired - H;
		const double Dist = ToTarget.Size();
		if (L1 < UE_KINDA_SMALL_NUMBER || L2 < UE_KINDA_SMALL_NUMBER || Dist < 1e-3)
		{
			return;
		}

		const FVector Axis = ToTarget / Dist;
		const double Reach = FMath::Clamp(Dist, FMath::Abs(L1 - L2) + 1e-3, L1 + L2 - 1e-3);

		// Knee: keep bending the way the original pose bends (pole = current knee offset perpendicular to the leg axis).
		FVector Pole = (K - H) - Axis * FVector::DotProduct(K - H, Axis);
		if (Pole.SizeSquared() < 1e-6)
		{
			Pole = FVector::CrossProduct(Axis, FVector::UpVector);
			if (Pole.SizeSquared() < 1e-6) { Pole = FVector::CrossProduct(Axis, FVector::RightVector); }
		}
		Pole.Normalize();
		if (PoleTarget && PoleWeight > 1e-4)
		{
			// A pole-vector control overrides which way the knee/elbow points.
			FVector Wanted = (*PoleTarget - H) - Axis * FVector::DotProduct(*PoleTarget - H, Axis);
			if (Wanted.SizeSquared() > 1e-6)
			{
				Wanted.Normalize();
				Pole = FMath::Lerp(Pole, Wanted, FMath::Clamp(PoleWeight, 0.0, 1.0)).GetSafeNormal();
			}
		}

		const double Along = (L1 * L1 - L2 * L2 + Reach * Reach) / (2.0 * Reach);
		const double Height = FMath::Sqrt(FMath::Max(0.0, L1 * L1 - Along * Along));
		const FVector NewK = H + Axis * Along + Pole * Height;
		const FVector NewA = H + Axis * Reach;

		// Thigh: swing the original knee direction onto the new one.
		const FQuat R1 = FQuat::FindBetweenNormals((K - H) / L1, (NewK - H).GetSafeNormal());
		const FQuat ThighRot = R1 * ThighCS.GetRotation();
		// Calf: carry the original rotation along with the thigh, then swing the shin onto the new ankle.
		const FVector ShinAfterThigh = R1.RotateVector(A - K);
		const FQuat R2 = FQuat::FindBetween(ShinAfterThigh.GetSafeNormal(), (NewA - NewK).GetSafeNormal());
		const FQuat CalfRot = R2 * R1 * CalfCS.GetRotation();

		const FTransform NewThighCS(ThighRot, H, ThighCS.GetScale3D());
		const FTransform NewCalfCS(CalfRot, NewK, CalfCS.GetScale3D());
		const FTransform NewFootCS(EffectorRotation ? *EffectorRotation : FootCS.GetRotation(), NewA, FootCS.GetScale3D()); // keep the foot's world orientation unless an effector rotation is given

		// Write back rotations only: translations and scales of the bones are unchanged.
		Local[Chain.Thigh].SetRotation(NewThighCS.GetRelativeTransform(ParentCS).GetRotation().GetNormalized());
		Local[Chain.Calf].SetRotation(NewCalfCS.GetRelativeTransform(NewThighCS).GetRotation().GetNormalized());
		Local[Chain.Foot].SetRotation(NewFootCS.GetRelativeTransform(NewCalfCS).GetRotation().GetNormalized());
	}
}

void FAnimStitchFootLock::SolveLimb(const FReferenceSkeleton& RefSkeleton, TArray<FTransform>& LocalPose, const FAnimStitchFootChain& Chain, const FVector& DesiredEffector,
	const FVector* PoleTarget, double PoleWeight, const FQuat* EffectorRotation)
{
	if (Chain.IsValid())
	{
		SolveLeg(RefSkeleton, LocalPose, Chain, DesiredEffector, PoleTarget, PoleWeight, EffectorRotation);
	}
}

FTransform FAnimStitchFootLock::GetComponentSpace(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& LocalPose, int32 Bone)
{
	return ComponentSpace(RefSkeleton, LocalPose, Bone);
}

int32 FAnimStitchFootLock::Pin(const FAnimStitchEvaluator& Evaluator, TArray<TArray<FTransform>>& Poses, const FAnimStitchFootLockResult& Detection, FAnimStitchPinOutput* Output)
{
	if (!Detection.bValid)
	{
		return 0;
	}
	const FAnimStitchFootLockSettings& S = Evaluator.GetTimeline().FootLock;
	const FReferenceSkeleton& Ref = Evaluator.GetSkeleton()->GetReferenceSkeleton();
	const int32 NumFrames = Poses.Num();
	const int32 BlendFrames = FMath::Max(1, FMath::RoundToInt(S.BlendTime * Detection.Fps));

	// Pass 1: where each foot's ankle should be on each frame (from the unmodified poses).
	struct FTarget { bool bActive = false; FVector Desired = FVector::ZeroVector; bool bRelease = false; double ReleaseWeight = 0.0; };
	TArray<FTarget> Targets[2];
	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		Targets[Foot].SetNum(NumFrames);
		if (Output)
		{
			Output->ManualTarget[Foot].Init(FVector::ZeroVector, NumFrames);
		}
		const FAnimStitchFootChain& Chain = Detection.Chains[Foot];
		const TArray<uint8>& Mask = Detection.Mask[Foot];

		for (int32 Start = 0; Start < NumFrames;)
		{
			if (!AnimStitchPlant::IsPlanted(Mask[Start])) { ++Start; continue; }
			int32 End = Start;
			while (End + 1 < NumFrames && Mask[End + 1] == Mask[Start]) { ++End; }

			if (Mask[Start] == AnimStitchPlant::ManualPin)
			{
				// Manual lock: glued to the foot's world position at the lock frame (root motion included) for as long as the
				// lock lasts - no automatic release. Only after an explicit unlock does the foot fade back to the animation.
				// (+ the offset the user dragged the pin by: a manual lock is a free IK target)
				const FVector Target = ComponentSpace(Ref, Poses[Start], Chain.Foot).GetTranslation()
					+ (Detection.PinOffset[Foot].IsValidIndex(Start) ? Detection.PinOffset[Foot][Start] : FVector::ZeroVector);
				for (int32 f = Start; f <= End; ++f)
				{
					Targets[Foot][f] = { true, Target };
					if (Output)
					{
						Output->ManualTarget[Foot][f] = Target;
					}
				}
				const bool bFollowedByPlant = End + 1 < NumFrames && AnimStitchPlant::IsPlanted(Mask[End + 1]);
				// After an explicit unlock the foot eases back to the animation. The further the body has moved away from the
				// locked spot, the longer this takes (the foot never snaps back faster than ~1.5 m/s).
				int32 ReleaseFrames = BlendFrames;
				if (End + 1 < NumFrames)
				{
					constexpr double MaxReleaseSpeed = 150.0; // cm/s
					const double Drift = (ComponentSpace(Ref, Poses[End + 1], Chain.Foot).GetTranslation() - Target).Size();
					ReleaseFrames = FMath::Clamp(FMath::CeilToInt32(Drift / (MaxReleaseSpeed / Detection.Fps)), BlendFrames, 24);
				}
				for (int32 k = 1; k <= ReleaseFrames && End + k < NumFrames && !bFollowedByPlant; ++k)
				{
					if (AnimStitchPlant::IsPlanted(Mask[End + k])) { break; }
					const double Alpha = 1.0 - k / (ReleaseFrames + 1.0);
					const double Weight = Alpha * Alpha * (3.0 - 2.0 * Alpha);
					const FVector Current = ComponentSpace(Ref, Poses[End + k], Chain.Foot).GetTranslation();
					Targets[Foot][End + k] = { true, FMath::Lerp(Current, Target, Weight), true, Weight };
				}
			}
			else
			{
				// Auto plant. Target: where the foot is on average during the plant (world space, root motion included).
				TArray<FVector> Original;
				FVector Mean = FVector::ZeroVector;
				for (int32 f = Start; f <= End; ++f)
				{
					Original.Add(ComponentSpace(Ref, Poses[f], Chain.Foot).GetTranslation());
					Mean += Original.Last();
				}
				Mean /= double(End - Start + 1);
				for (int32 f = Start; f <= End; ++f)
				{
					const int32 FromEdge = FMath::Min(f - Start, End - f);
					const double Alpha = FMath::Clamp((FromEdge + 1.0) / (BlendFrames + 1.0), 0.0, 1.0);
					const double Weight = Alpha * Alpha * (3.0 - 2.0 * Alpha);
					const FVector& P = Original[f - Start];
					Targets[Foot][f] = { true, FVector(FMath::Lerp(P.X, Mean.X, Weight), FMath::Lerp(P.Y, Mean.Y, Weight), P.Z) };
				}
			}
			Start = End + 1;
		}
	}

	// Pass 2: pelvis follows. Where a pinned foot would be beyond (about 97% of) the leg's reach, move the pelvis the minimum
	// amount that brings every pinned foot back in reach, capped and lightly smoothed. Spine and head follow the pelvis.
	const int32 Pelvis = Detection.Chains[0].ThighParent == Detection.Chains[1].ThighParent ? Detection.Chains[0].ThighParent : INDEX_NONE;
	TArray<FVector> PelvisOffset;
	PelvisOffset.Init(FVector::ZeroVector, NumFrames);
	bool bAnyOffset = false;
	if (S.bPelvisFollows && Pelvis != INDEX_NONE && S.MaxPelvisOffset > 0.f)
	{
		constexpr double ReachFraction = 0.97;
		for (int32 f = 0; f < NumFrames; ++f)
		{
			FVector Hip[2], Goal[2];
			double Reach[2];
			bool bUse[2];
			for (int32 Foot = 0; Foot < 2; ++Foot)
			{
				const FAnimStitchFootChain& Chain = Detection.Chains[Foot];
				bUse[Foot] = Targets[Foot][f].bActive && !Targets[Foot][f].bRelease;
				if (!bUse[Foot]) { continue; }
				const FVector H = ComponentSpace(Ref, Poses[f], Chain.Thigh).GetTranslation();
				const FVector K = ComponentSpace(Ref, Poses[f], Chain.Calf).GetTranslation();
				const FVector A = ComponentSpace(Ref, Poses[f], Chain.Foot).GetTranslation();
				Hip[Foot] = H;
				Goal[Foot] = Targets[Foot][f].Desired;
				// Never ask for more bend than the animation itself had: a leg that was already nearly straight stays as it was.
				Reach[Foot] = FMath::Max(((K - H).Size() + (A - K).Size()) * ReachFraction, (A - H).Size());
			}
			if (!bUse[0] && !bUse[1]) { continue; }

			FVector Offset = FVector::ZeroVector;
			for (int32 Iteration = 0; Iteration < 6; ++Iteration)
			{
				for (int32 Foot = 0; Foot < 2; ++Foot)
				{
					if (!bUse[Foot]) { continue; }
					const FVector FromGoal = Hip[Foot] + Offset - Goal[Foot];
					const double Dist = FromGoal.Size();
					if (Dist > Reach[Foot])
					{
						Offset -= FromGoal * (1.0 - Reach[Foot] / Dist);
					}
				}
			}
			const double Cap = S.MaxPelvisOffset;
			if (Offset.Size() > Cap) { Offset *= Cap / Offset.Size(); }
			PelvisOffset[f] = Offset;
			bAnyOffset |= !Offset.IsNearlyZero(1e-4);
		}

		// While a foot eases back to the animation after an explicit unlock, the pelvis offset it needed eases out with it.
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			for (int32 f = 1; f < NumFrames; ++f)
			{
				if (!Targets[Foot][f].bRelease || Targets[Foot][f - 1].bRelease) { continue; }
				const FVector Start = PelvisOffset[f - 1];
				if (Start.IsNearlyZero(1e-4)) { continue; }
				for (int32 g = f; g < NumFrames && Targets[Foot][g].bRelease; ++g)
				{
					const FVector Decay = Start * Targets[Foot][g].ReleaseWeight;
					if (Decay.SizeSquared() > PelvisOffset[g].SizeSquared()) { PelvisOffset[g] = Decay; bAnyOffset = true; }
				}
			}
		}

		if (bAnyOffset)
		{
			// Light 5-tap smoothing so the body never jerks.
			TArray<FVector> Smoothed = PelvisOffset;
			for (int32 f = 0; f < NumFrames; ++f)
			{
				FVector Sum = FVector::ZeroVector;
				double Weight = 0.0;
				for (int32 k = -2; k <= 2; ++k)
				{
					const int32 g = f + k;
					if (g < 0 || g >= NumFrames) { continue; }
					const double W = 3.0 - FMath::Abs(k);
					Sum += PelvisOffset[g] * W;
					Weight += W;
				}
				Smoothed[f] = Sum / Weight;
			}
			for (int32 f = 0; f < NumFrames; ++f)
			{
				if (Smoothed[f].IsNearlyZero(1e-4)) { continue; }
				const FTransform ParentCS = Ref.GetParentIndex(Pelvis) != INDEX_NONE ? ComponentSpace(Ref, Poses[f], Ref.GetParentIndex(Pelvis)) : FTransform::Identity;
				Poses[f][Pelvis].SetTranslation(Poses[f][Pelvis].GetTranslation() + ParentCS.InverseTransformVector(Smoothed[f]));
			}
		}
	}

	// Pass 3: solve both legs onto their targets (with the hips where pass 2 left them).
	int32 Corrected = 0;
	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		for (int32 f = 0; f < NumFrames; ++f)
		{
			if (Targets[Foot][f].bActive)
			{
				SolveLeg(Ref, Poses[f], Detection.Chains[Foot], Targets[Foot][f].Desired);
				++Corrected;
			}
		}
	}
	return Corrected;
}
FAnimStitchPredictReport FAnimStitchFootLock::PredictFeet(const FAnimStitchEvaluator& Evaluator, TArray<TArray<FTransform>>& Poses, double Fps)
{
	FAnimStitchPredictReport Report;
	const FAnimStitchTimeline& Timeline = Evaluator.GetTimeline();
	const FAnimStitchFootLockSettings& S = Timeline.FootLock;
	const USkeleton* Skeleton = Evaluator.GetSkeleton();
	const int32 NumFrames = Poses.Num();
	if (!Skeleton || NumFrames < 2 || Fps <= 0.0)
	{
		Report.Problem = LOCTEXT("PredictNoData", "Foot prediction has nothing to analyse.");
		return Report;
	}
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();

	FName Bones[2];
	ResolveFootBones(Skeleton, S, Bones[0], Bones[1]);
	FAnimStitchFootChain Chains[2];
	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		Chains[Foot] = BuildChain(Ref, Bones[Foot]);
		if (!Chains[Foot].IsValid())
		{
			Report.Problem = FText::Format(LOCTEXT("PredictNoChain", "Foot prediction: the foot bone '{0}' was not found or has no thigh/calf above it. Pick the foot bones in the Foot Lock settings."), FText::FromName(Bones[Foot]));
			return Report;
		}
	}
	double GroundZ = S.GroundHeight;
	if (S.bAutoGroundHeight)
	{
		const TArray<FTransform>& RefPose = Ref.GetRefBonePose();
		GroundZ = FMath::Min(ComponentSpace(Ref, RefPose, Chains[0].Foot).GetTranslation().Z, ComponentSpace(Ref, RefPose, Chains[1].Foot).GetTranslation().Z);
	}
	Report.bValid = true;

	const double Dt = 1.0 / Fps;
	const int32 MinFrames = FMath::Max(1, FMath::CeilToInt(S.MinPlantDuration * Fps));
	const int32 BlendFrames = FMath::Max(1, FMath::RoundToInt(S.BlendTime * Fps));

	// 1) Plant timeline of every clip that takes part in a crossfade, from that clip's own unmixed motion.
	struct FClipFoot
	{
		bool bAnalysed = false;
		int32 First = 0;
		TArray<FVector> Pin;       // per frame: pin position of the plant the frame belongs to
		TArray<uint8> Planted;
		bool IsPlanted(int32 Frame) const { const int32 i = Frame - First; return Planted.IsValidIndex(i) && Planted[i]; }
		FVector PinAt(int32 Frame) const { return Pin[Frame - First]; }
	};
	const int32 NumClips = Timeline.Clips.Num();
	TArray<FClipFoot> ClipFeet[2];
	ClipFeet[0].SetNum(NumClips);
	ClipFeet[1].SetNum(NumClips);

	auto AnalyseClip = [&](int32 ClipIndex)
	{
		if (ClipFeet[0][ClipIndex].bAnalysed) { return; }
		const FAnimStitchClip& Clip = Timeline.Clips[ClipIndex];
		const int32 F0 = FMath::Max(0, FMath::CeilToInt(Clip.StartTime * Fps - 1e-6));
		const int32 F1 = FMath::Min(NumFrames - 1, FMath::FloorToInt(Clip.GetEndTime() * Fps + 1e-6));
		const int32 N = F1 - F0 + 1;
		if (N < 2) { return; }

		TArray<FVector> Position[2];
		Position[0].SetNum(N);
		Position[1].SetNum(N);
		TArray<FTransform> Pose;
		for (int32 i = 0; i < N; ++i)
		{
			Evaluator.EvaluateClipPose(ClipIndex, (F0 + i) * Dt, Pose);
			for (int32 Foot = 0; Foot < 2; ++Foot)
			{
				Position[Foot][i] = ComponentSpace(Ref, Pose, Chains[Foot].Foot).GetTranslation();
			}
		}
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			FClipFoot& Out = ClipFeet[Foot][ClipIndex];
			Out.bAnalysed = true;
			Out.First = F0;
			Out.Planted.Init(0, N);
			Out.Pin.Init(FVector::ZeroVector, N);
			for (int32 i = 0; i < N; ++i)
			{
				const int32 Prev = FMath::Max(i - 1, 0), Next = FMath::Min(i + 1, N - 1);
				const double Speed = (Position[Foot][Next] - Position[Foot][Prev]).Size() / ((Next - Prev) * Dt);
				Out.Planted[i] = (Position[Foot][i].Z - GroundZ <= S.HeightThreshold) && Speed <= S.VelocityThreshold ? 1 : 0;
			}
			for (int32 i = 0; i < N;)
			{
				if (!Out.Planted[i]) { ++i; continue; }
				int32 End = i;
				while (End + 1 < N && Out.Planted[End + 1]) { ++End; }
				if (End - i + 1 < MinFrames)
				{
					for (int32 k = i; k <= End; ++k) { Out.Planted[k] = 0; }
				}
				else
				{
					FVector Mean = FVector::ZeroVector;
					for (int32 k = i; k <= End; ++k) { Mean += Position[Foot][k]; }
					Mean /= double(End - i + 1);
					for (int32 k = i; k <= End; ++k) { Out.Pin[k] = Mean; }
				}
				i = End + 1;
			}
		}
	};

	// 2) Inside each crossfade, decide where each foot should be.
	struct FTarget { double Weight = 0.0; FVector Desired = FVector::ZeroVector; FVector Target = FVector::ZeroVector; };
	TArray<FTarget> Targets[2];
	Targets[0].SetNum(NumFrames);
	Targets[1].SetNum(NumFrames);

	for (int32 In = 1; In < NumClips; ++In)
	{
		const FAnimStitchClip& B = Timeline.Clips[In];
		const FAnimStitchClip& A = Timeline.Clips[In - 1];
		if (B.OverlapIn <= 1e-4) { continue; } // only crossfades are predicted; butt joins have nothing to blend
		AnalyseClip(In - 1);
		AnalyseClip(In);
		const int32 WinStart = FMath::Max(0, FMath::CeilToInt(B.StartTime * Fps - 1e-6));
		const int32 WinEnd = FMath::Min(NumFrames - 1, FMath::FloorToInt(A.GetEndTime() * Fps + 1e-6));
		if (WinEnd < WinStart) { continue; }
		++Report.Overlaps;

		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			const FAnimStitchFootChain& Chain = Chains[Foot];
			const FClipFoot& FeetA = ClipFeet[Foot][In - 1];
			const FClipFoot& FeetB = ClipFeet[Foot][In];

			// Raw target per frame (unweighted).
			TArray<uint8> Active;
			Active.Init(0, NumFrames);
			TArray<FVector> Wanted;
			Wanted.Init(FVector::ZeroVector, NumFrames);
			TArray<double> Extra; // extra weight factor per frame (1 unless the target should only take over as the blend progresses)
			Extra.Init(1.0, NumFrames);
			for (int32 f = WinStart; f <= WinEnd; ++f)
			{
				const bool bA = FeetA.IsPlanted(f), bB = FeetB.IsPlanted(f);
				if (!bA && !bB) { continue; }
				const double Alpha = Evaluator.EvaluateBlendAlpha(In, f * Dt);
				FVector T;
				if (bA && bB)  { T = FMath::Lerp(FeetA.PinAt(f), FeetB.PinAt(f), Alpha); } // both planted: slide the pin from the old stance to the new one
				else if (bA)   { T = FeetA.PinAt(f); }                                      // hold the outgoing plant
				else           { T = FeetB.PinAt(f); Extra[f] = Alpha; }                    // settle into the incoming stance, gradually as the blend progresses
				Wanted[f] = T;
				Active[f] = 1;
			}

			// Weights: ease in (not at the start of the crossfade), ease out after the target ends, and release when the pelvis is out of reach.
			for (int32 Start = WinStart; Start <= WinEnd;)
			{
				if (!Active[Start]) { ++Start; continue; }
				int32 End = Start;
				while (End + 1 <= WinEnd && Active[End + 1]) { ++End; }

				auto Apply = [&](int32 f, double RampWeight)
				{
					const FVector Hip = ComponentSpace(Ref, Poses[f], Chain.Thigh).GetTranslation();
					const FVector Knee = ComponentSpace(Ref, Poses[f], Chain.Calf).GetTranslation();
					const FVector Ankle = ComponentSpace(Ref, Poses[f], Chain.Foot).GetTranslation();
					const double Length = (Knee - Hip).Size() + (Ankle - Knee).Size();
					const FVector Want(Wanted[FMath::Min(f, End)].X, Wanted[FMath::Min(f, End)].Y, Ankle.Z);
					// Pelvis read-only: fade the pin out only as the hip-to-target distance reaches the leg's full length.
					const double Ratio = (Want - Hip).Size() / FMath::Max(Length, 1e-3);
					const double ReachWeight = 1.0 - FMath::SmoothStep(0.97, 0.995, Ratio); // a standing leg is already ~95% extended, so only fade near the very end of the reach
					const double Weight = FMath::Clamp(RampWeight * ReachWeight * Extra[FMath::Min(f, End)], 0.0, 1.0);
					if (ReachWeight < 0.999) { ++Report.ReachReleasedFrames; }
					FTarget& Slot = Targets[Foot][f];
					if (Weight > Slot.Weight)
					{
						Slot.Weight = Weight;
						Slot.Target = Want;
						Slot.Desired = FMath::Lerp(Ankle, Want, Weight);
					}
				};

				for (int32 f = Start; f <= End; ++f)
				{
					double Ramp = 1.0;
					if (Start != WinStart) // the crossfade starts on the outgoing clip's own plant: no ramp needed there
					{
						const double R = FMath::Clamp((f - Start + 1.0) / (BlendFrames + 1.0), 0.0, 1.0);
						Ramp = R * R * (3.0 - 2.0 * R);
					}
					Apply(f, Ramp);
				}
				// Ease back to the blended motion after the target ends; the further the foot has to travel, the longer it takes.
				if (End + 1 < NumFrames)
				{
					constexpr double MaxReleaseSpeed = 150.0; // cm/s
					const FVector Held = Targets[Foot][End].Target;
					const double Drift = FVector2D(ComponentSpace(Ref, Poses[End + 1], Chain.Foot).GetTranslation() - Held).Size();
					const int32 ReleaseFrames = FMath::Clamp(FMath::CeilToInt32(Drift / (MaxReleaseSpeed / Fps)), BlendFrames, 24);
					for (int32 k = 1; k <= ReleaseFrames && End + k < NumFrames && !Active[End + k]; ++k)
					{
						const double R = 1.0 - k / (ReleaseFrames + 1.0);
						Apply(End + k, R * R * (3.0 - 2.0 * R));
					}
				}
				Start = End + 1;
			}
		}
	}

	// 3) Solve the legs onto the targets. Only leg rotations change; the pelvis is untouched.
	for (int32 Foot = 0; Foot < 2; ++Foot)
	{
		for (int32 f = 0; f < NumFrames; ++f)
		{
			const FTarget& Slot = Targets[Foot][f];
			if (Slot.Weight <= 1e-3) { continue; }
			const FVector Before = ComponentSpace(Ref, Poses[f], Chains[Foot].Foot).GetTranslation();
			SolveLeg(Ref, Poses[f], Chains[Foot], Slot.Desired);
			++Report.PinnedFootFrames;
			if (Slot.Weight > 0.99)
			{
				const FVector After = ComponentSpace(Ref, Poses[f], Chains[Foot].Foot).GetTranslation();
				Report.MaxDeviationBefore = FMath::Max(Report.MaxDeviationBefore, FVector2D(Before - Slot.Target).Size());
				Report.MaxDeviationAfter = FMath::Max(Report.MaxDeviationAfter, FVector2D(After - Slot.Target).Size());
			}
		}
	}
	return Report;
}
#undef LOCTEXT_NAMESPACE











