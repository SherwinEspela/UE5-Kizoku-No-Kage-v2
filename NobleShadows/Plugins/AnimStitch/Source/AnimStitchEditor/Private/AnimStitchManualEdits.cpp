#include "AnimStitchManualEdits.h"
#include "ReferenceSkeleton.h"

namespace
{
	int32 BoneDepth(const FReferenceSkeleton& Ref, int32 Bone)
	{
		int32 Depth = 0;
		for (int32 P = Ref.GetParentIndex(Bone); P != INDEX_NONE; P = Ref.GetParentIndex(P)) { ++Depth; }
		return Depth;
	}

	bool SideMatch(const FString& Lower, bool bLeft)
	{
		if (bLeft)
		{
			return Lower.Contains(TEXT("left")) || Lower.EndsWith(TEXT("_l")) || Lower.EndsWith(TEXT(".l")) || Lower.Contains(TEXT("_l_")) || Lower.Contains(TEXT(" l ")) || Lower.StartsWith(TEXT("l_"));
		}
		return Lower.Contains(TEXT("right")) || Lower.EndsWith(TEXT("_r")) || Lower.EndsWith(TEXT(".r")) || Lower.Contains(TEXT("_r_")) || Lower.Contains(TEXT(" r ")) || Lower.StartsWith(TEXT("r_"));
	}

	bool HasWord(const FString& Lower, const TCHAR* Word)
	{
		return Lower.Contains(Word);
	}
}

int32 FAnimStitchManualEdits::GetBoneSide(FName Bone)
{
	const FString Lower = Bone.ToString().ToLower();
	if (SideMatch(Lower, true)) { return -1; }
	if (SideMatch(Lower, false)) { return 1; }
	return 0;
}

bool FAnimStitchManualEdits::IsIdentityKey(const FAnimStitchTrackKey& Key)
{
	return Key.Translation.IsNearlyZero(1e-4) && Key.Rotation.Equals(FQuat::Identity, 1e-5) && Key.Scale.Equals(FVector::OneVector, 1e-5);
}

namespace
{
	/** IK keys are events: the weight holds until the next key, then ramps over this long. */
	constexpr double IKWeightRamp = 0.1;

	FAnimStitchTrackKey SampleIK(const FAnimStitchTrack& Track, double Time)
	{
		const TArray<FAnimStitchTrackKey>& Keys = Track.Keys;
		int32 I = INDEX_NONE;
		for (int32 k = 0; k < Keys.Num() && Keys[k].Time <= Time; ++k) { I = k; }
		if (I == INDEX_NONE)
		{
			FAnimStitchTrackKey Out = Keys[0];
			Out.Weight = 0.f; // before the first key the animation plays
			return Out;
		}
		const FAnimStitchTrackKey& K = Keys[I];
		const float PreviousWeight = I > 0 ? Keys[I - 1].Weight : 0.f;
		const double Alpha = FMath::Clamp((Time - K.Time) / IKWeightRamp, 0.0, 1.0);
		FAnimStitchTrackKey Out = K;
		Out.Time = Time;
		Out.Weight = FMath::Lerp(PreviousWeight, K.Weight, float(Alpha));
		// Target: between two active keys it interpolates; a key with weight 0 has no target of its own, so it borrows the neighbour's.
		if (K.Weight > 1e-3f && I + 1 < Keys.Num() && Keys[I + 1].Weight > 1e-3f)
		{
			const FAnimStitchTrackKey& N = Keys[I + 1];
			const double Span = N.Time - K.Time;
			const double T = Span > 1e-9 ? (Time - K.Time) / Span : 0.0;
			Out.Translation = FMath::Lerp(K.Translation, N.Translation, T);
			Out.Rotation = FQuat::Slerp(K.Rotation, N.Rotation, T).GetNormalized();
			Out.Pole = FMath::Lerp(K.Pole, N.Pole, T);
		}
		else if (K.Weight <= 1e-3f && I > 0 && Keys[I - 1].Weight > 1e-3f)
		{
			Out.Translation = Keys[I - 1].Translation;
			Out.Rotation = Keys[I - 1].Rotation;
			Out.Pole = Keys[I - 1].Pole;
		}
		return Out;
	}
}

FAnimStitchTrackKey FAnimStitchManualEdits::Sample(const FAnimStitchTrack& Track, double Time)
{
	if (Track.Keys.Num() == 0)
	{
		return FAnimStitchTrackKey();
	}
	if (Track.Kind == EAnimStitchTrackKind::IKLimb)
	{
		return SampleIK(Track, Time);
	}
	if (Time <= Track.Keys[0].Time) { return Track.Keys[0]; }
	if (Time >= Track.Keys.Last().Time) { return Track.Keys.Last(); }
	for (int32 i = 0; i + 1 < Track.Keys.Num(); ++i)
	{
		const FAnimStitchTrackKey& A = Track.Keys[i];
		const FAnimStitchTrackKey& B = Track.Keys[i + 1];
		if (Time >= A.Time && Time <= B.Time)
		{
			const double Span = B.Time - A.Time;
			const double Alpha = Span > 1e-9 ? (Time - A.Time) / Span : 0.0;
			FAnimStitchTrackKey Out;
			Out.Time = Time;
			Out.Translation = FMath::Lerp(A.Translation, B.Translation, Alpha);
			Out.Rotation = FQuat::Slerp(A.Rotation, B.Rotation, Alpha).GetNormalized();
			Out.Scale = FMath::Lerp(A.Scale, B.Scale, Alpha);
			Out.Pole = FMath::Lerp(A.Pole, B.Pole, Alpha);
			Out.Weight = FMath::Lerp(A.Weight, B.Weight, float(Alpha));
			return Out;
		}
	}
	return Track.Keys.Last();
}

int32 FAnimStitchManualEdits::FindKey(const FAnimStitchTrack& Track, double Time, double Tolerance)
{
	for (int32 i = 0; i < Track.Keys.Num(); ++i)
	{
		if (FMath::Abs(Track.Keys[i].Time - Time) <= Tolerance) { return i; }
	}
	return INDEX_NONE;
}

int32 FAnimStitchManualEdits::SetKey(FAnimStitchTrack& Track, const FAnimStitchTrackKey& Key)
{
	const int32 Existing = FindKey(Track, Key.Time);
	if (Existing != INDEX_NONE)
	{
		const double KeepTime = Track.Keys[Existing].Time;
		Track.Keys[Existing] = Key;
		Track.Keys[Existing].Time = KeepTime;
		return Existing;
	}
	int32 Index = 0;
	while (Index < Track.Keys.Num() && Track.Keys[Index].Time < Key.Time) { ++Index; }
	Track.Keys.Insert(Key, Index);
	return Index;
}

FAnimStitchFootChain FAnimStitchManualEdits::ChainFor(const FReferenceSkeleton& RefSkeleton, FName EffectorBone)
{
	return FAnimStitchFootLock::BuildChain(RefSkeleton, EffectorBone);
}

FVector FAnimStitchManualEdits::DefaultPole(const FReferenceSkeleton& RefSkeleton, const FAnimStitchFootChain& Chain, const TArray<FTransform>& LocalPose)
{
	if (!Chain.IsValid()) { return FVector::ZeroVector; }
	const FVector H = FAnimStitchFootLock::GetComponentSpace(RefSkeleton, LocalPose, Chain.Thigh).GetTranslation();
	const FVector K = FAnimStitchFootLock::GetComponentSpace(RefSkeleton, LocalPose, Chain.Calf).GetTranslation();
	const FVector A = FAnimStitchFootLock::GetComponentSpace(RefSkeleton, LocalPose, Chain.Foot).GetTranslation();
	FVector Out = K - (H + A) * 0.5;
	if (Out.SizeSquared() < 1e-4) { Out = FVector::ForwardVector; }
	return K + Out.GetSafeNormal() * 40.0;
}

FName FAnimStitchManualEdits::GuessHandBone(const FReferenceSkeleton& RefSkeleton, bool bLeft)
{
	static const TCHAR* Excluded[] = { TEXT("ik"), TEXT("twist"), TEXT("roll"), TEXT("corrective"), TEXT("helper"), TEXT("pole"), TEXT("end") };
	FName Best;
	int32 BestScore = -1;
	for (int32 i = 0; i < RefSkeleton.GetNum(); ++i)
	{
		const FString Lower = RefSkeleton.GetBoneName(i).ToString().ToLower();
		if (!Lower.Contains(TEXT("hand")) || !SideMatch(Lower, bLeft)) { continue; }
		bool bBad = false;
		for (const TCHAR* Word : Excluded) { bBad |= HasWord(Lower, Word) && (FCString::Strcmp(Word, TEXT("ik")) != 0 || Lower.StartsWith(TEXT("ik")) || Lower.Contains(TEXT("_ik")) || Lower.Contains(TEXT("ik_"))); }
		if (bBad) { continue; }
		const int32 Score = 1000 - Lower.Len();
		if (Score > BestScore) { BestScore = Score; Best = RefSkeleton.GetBoneName(i); }
	}
	return Best;
}

TArray<FName> FAnimStitchManualEdits::GetCoreBoneSuggestions(const FReferenceSkeleton& RefSkeleton)
{
	static const TCHAR* Wanted[] = { TEXT("pelvis"), TEXT("hips"), TEXT("spine"), TEXT("chest"), TEXT("neck"), TEXT("head") };
	static const TCHAR* Excluded[] = { TEXT("twist"), TEXT("corrective"), TEXT("roll"), TEXT("helper"), TEXT("_ik"), TEXT("ik_"), TEXT("end"), TEXT("jaw"), TEXT("eye"), TEXT("tongue"), TEXT("teeth"), TEXT("hair"), TEXT("brow"), TEXT("lip"), TEXT("cheek"), TEXT("nose"), TEXT("ear"), TEXT("facial") };
	TArray<FName> Result;
	for (int32 i = 0; i < RefSkeleton.GetNum() && Result.Num() < 14; ++i)
	{
		const FString Lower = RefSkeleton.GetBoneName(i).ToString().ToLower();
		bool bMatch = false;
		for (const TCHAR* W : Wanted) { bMatch |= Lower.Contains(W); }
		bool bBad = false;
		for (const TCHAR* W : Excluded) { bBad |= Lower.Contains(W); }
		if (bMatch && !bBad && BoneDepth(RefSkeleton, i) < 9) { Result.Add(RefSkeleton.GetBoneName(i)); }
	}
	return Result;
}

FVector FAnimStitchManualEdits::DefaultPoleForPose(const FReferenceSkeleton& Ref, const FAnimStitchFootChain& Chain, const TArray<FTransform>& ComponentPose)
{
	if (!Chain.IsValid() || !ComponentPose.IsValidIndex(Chain.Foot)) { return FVector::ZeroVector; }
	auto BendOffset = [&](const FVector& H, const FVector& K, const FVector& A)
	{
		const FVector Axis = (A - H).GetSafeNormal();
		return (K - H) - Axis * FVector::DotProduct(K - H, Axis); // knee/elbow position relative to the hip-ankle line
	};
	const FVector K = ComponentPose[Chain.Calf].GetTranslation();
	FVector Out = BendOffset(ComponentPose[Chain.Thigh].GetTranslation(), K, ComponentPose[Chain.Foot].GetTranslation());
	if (Out.SizeSquared() < 4.0) // straight limb: use the way the reference pose bends
	{
		const TArray<FTransform>& RefLocal = Ref.GetRefBonePose();
		Out = BendOffset(FAnimStitchFootLock::GetComponentSpace(Ref, RefLocal, Chain.Thigh).GetTranslation(), FAnimStitchFootLock::GetComponentSpace(Ref, RefLocal, Chain.Calf).GetTranslation(),
			FAnimStitchFootLock::GetComponentSpace(Ref, RefLocal, Chain.Foot).GetTranslation());
	}
	if (Out.SizeSquared() < 1e-4) { Out = FVector::ForwardVector; }
	return K + Out.GetSafeNormal() * 40.0;
}

void FAnimStitchManualEdits::Apply(const FReferenceSkeleton& Ref, const TArray<FAnimStitchTrack>& Tracks, double Time, TArray<FTransform>& Local, const FName* PlantFootBones)
{
	if (Tracks.Num() == 0) { return; }

	// Bone (additive) tracks first, parents before children; then IK limbs on top of the corrected body.
	TArray<TPair<int32, const FAnimStitchTrack*>> BoneTracks;
	for (const FAnimStitchTrack& Track : Tracks)
	{
		const int32 Bone = Ref.FindBoneIndex(Track.Bone);
		if (Track.Kind == EAnimStitchTrackKind::Bone && Bone != INDEX_NONE && Track.Keys.Num() > 0)
		{
			BoneTracks.Add({ BoneDepth(Ref, Bone), &Track });
		}
	}
	BoneTracks.StableSort([](const TPair<int32, const FAnimStitchTrack*>& A, const TPair<int32, const FAnimStitchTrack*>& B) { return A.Key < B.Key; });

	// Feet to keep planted: legs whose hip chain is moved by a bone track (pelvis...) and that no active IK track owns.
	struct FPlant { FAnimStitchFootChain Chain; FTransform Foot; };
	TArray<FPlant> Plants;
	if (PlantFootBones)
	{
		for (int32 f = 0; f < 2; ++f)
		{
			const FAnimStitchFootChain Chain = ChainFor(Ref, PlantFootBones[f]);
			if (!Chain.IsValid()) { continue; }
			bool bIKOwned = false;
			for (const FAnimStitchTrack& T : Tracks)
			{
				if (T.Kind == EAnimStitchTrackKind::IKLimb && T.Bone == PlantFootBones[f] && T.Keys.Num() > 0 && Sample(T, Time).Weight > 1e-3f) { bIKOwned = true; }
			}
			bool bMoved = false;
			for (const TPair<int32, const FAnimStitchTrack*>& Entry : BoneTracks)
			{
				if (!Entry.Value->bKeepFeetPlanted || IsIdentityKey(Sample(*Entry.Value, Time))) { continue; }
				const int32 TrackBone = Ref.FindBoneIndex(Entry.Value->Bone);
				for (int32 B = Chain.Thigh; B != INDEX_NONE; B = Ref.GetParentIndex(B)) { bMoved |= (B == TrackBone); }
			}
			if (!bIKOwned && bMoved) { Plants.Add({ Chain, FAnimStitchFootLock::GetComponentSpace(Ref, Local, Chain.Foot) }); }
		}
	}

	for (const TPair<int32, const FAnimStitchTrack*>& Entry : BoneTracks)
	{
		const FAnimStitchTrack& Track = *Entry.Value;
		const FAnimStitchTrackKey Key = Sample(Track, Time);
		if (IsIdentityKey(Key)) { continue; }
		const int32 Bone = Ref.FindBoneIndex(Track.Bone);
		const int32 Parent = Ref.GetParentIndex(Bone);
		const FTransform ParentCS = Parent != INDEX_NONE ? FAnimStitchFootLock::GetComponentSpace(Ref, Local, Parent) : FTransform::Identity;
		FTransform& L = Local[Bone];
		if (!Track.bRotationOnly)
		{
			L.SetTranslation(L.GetTranslation() + ParentCS.InverseTransformVector(Key.Translation));
			L.SetScale3D(L.GetScale3D() * Key.Scale);
		}
		if (!Key.Rotation.IsIdentity(1e-6))
		{
			// The delta is a component-space rotation about the bone: conjugate it into the parent's space.
			const FQuat ParentRot = ParentCS.GetRotation();
			L.SetRotation(((ParentRot.Inverse() * Key.Rotation * ParentRot) * L.GetRotation()).GetNormalized());
		}
	}

	for (const FPlant& Plant : Plants)
	{
		const FVector Knee = FAnimStitchFootLock::GetComponentSpace(Ref, Local, Plant.Chain.Calf).GetTranslation();
		const FQuat Rotation = Plant.Foot.GetRotation();
		FAnimStitchFootLock::SolveLimb(Ref, Local, Plant.Chain, Plant.Foot.GetTranslation(), &Knee, 1.0, &Rotation);
	}

	for (const FAnimStitchTrack& Track : Tracks)
	{
		if (Track.Kind != EAnimStitchTrackKind::IKLimb || Track.Keys.Num() == 0) { continue; }
		const FAnimStitchFootChain Chain = ChainFor(Ref, Track.Bone);
		if (!Chain.IsValid()) { continue; }
		const FAnimStitchTrackKey Key = Sample(Track, Time);
		const double Weight = FMath::Clamp((double)Key.Weight, 0.0, 1.0);
		if (Weight <= 1e-3) { continue; }
		const FTransform Effector = FAnimStitchFootLock::GetComponentSpace(Ref, Local, Chain.Foot);
		const FVector Desired = FMath::Lerp(Effector.GetTranslation(), Key.Translation, Weight);
		const FQuat Rotation = FQuat::Slerp(Effector.GetRotation(), Key.Rotation, Weight).GetNormalized();
		const FVector* Pole = Key.Pole.IsNearlyZero(1e-3) ? nullptr : &Key.Pole; // no pole stored: let the solver keep the animated bend
		FAnimStitchFootLock::SolveLimb(Ref, Local, Chain, Desired, Pole, Pole ? Weight : 0.0, &Rotation);
	}
}

