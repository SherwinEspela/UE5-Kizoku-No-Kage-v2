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

bool FAnimStitchManualEdits::IsPelvisBone(FName Bone)
{
	const FString Lower = Bone.ToString().ToLower();
	if (Lower.Contains(TEXT("twist")) || Lower.Contains(TEXT("ik")) || Lower.Contains(TEXT("corrective"))) { return false; }
	return Lower.Contains(TEXT("pelvis")) || Lower == TEXT("hips") || Lower == TEXT("hip");
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

	/**
	 * IK keys: a key's weight is reached AT its own time (the ramp comes just before it) and held until the next key's ramp begins.
	 * So a key written at the playhead is fully in effect on that very frame (what dragging a control needs).
	 */
	FAnimStitchTrackKey SampleIK(const FAnimStitchTrack& Track, double Time)
	{
		const TArray<FAnimStitchTrackKey>& Keys = Track.Keys;
		int32 I = INDEX_NONE;
		for (int32 k = 0; k < Keys.Num() && Keys[k].Time <= Time; ++k) { I = k; }
		const FAnimStitchTrackKey* K = Keys.IsValidIndex(I) ? &Keys[I] : nullptr;
		const FAnimStitchTrackKey* Next = Keys.IsValidIndex(I + 1) ? &Keys[I + 1] : nullptr;

		float Weight = K ? K->Weight : 0.f;
		if (Next)
		{
			const double Gap = K ? Next->Time - K->Time : Next->Time;
			const double Ramp = FMath::Max(FMath::Min(IKWeightRamp, Gap), 1e-6);
			const double RampStart = Next->Time - Ramp;
			if (Time > RampStart)
			{
				Weight = FMath::Lerp(Weight, Next->Weight, float(FMath::Clamp((Time - RampStart) / Ramp, 0.0, 1.0)));
			}
		}

		const bool bActiveK = K && K->Weight > 1e-3f;
		const bool bActiveNext = Next && Next->Weight > 1e-3f;
		FAnimStitchTrackKey Out = K ? *K : Keys[0];
		if (bActiveK && bActiveNext)
		{
			// Between two active keys the target interpolates.
			const double Span = Next->Time - K->Time;
			const double T = Span > 1e-9 ? (Time - K->Time) / Span : 0.0;
			Out.Translation = FMath::Lerp(K->Translation, Next->Translation, T);
			Out.Rotation = FQuat::Slerp(K->Rotation, Next->Rotation, T).GetNormalized();
			Out.Pole = FMath::Lerp(K->Pole, Next->Pole, T);
		}
		else if (!bActiveK && bActiveNext)
		{
			// An inactive key has no target of its own: ease in toward the next key's.
			Out.Translation = Next->Translation;
			Out.Rotation = Next->Rotation;
			Out.Pole = Next->Pole;
		}
		else if (!bActiveK && I > 0 && Keys[I - 1].Weight > 1e-3f)
		{
			// Released: keep the previous target while the weight eases off.
			Out.Translation = Keys[I - 1].Translation;
			Out.Rotation = Keys[I - 1].Rotation;
			Out.Pole = Keys[I - 1].Pole;
		}
		Out.Time = Time;
		Out.Weight = Weight;
		return Out;
	}
}

uint8 FAnimStitchManualEdits::GetApplicableChannels(const FAnimStitchTrack& Track)
{
	return Track.Kind == EAnimStitchTrackKind::Bone && Track.bRotationOnly ? AnimStitchChannel::Rotation : AnimStitchChannel::All;
}

uint8 FAnimStitchManualEdits::GetEditableChannels(const FAnimStitchTrack& Track)
{
	if (Track.Kind != EAnimStitchTrackKind::Bone) { return AnimStitchChannel::All; }
	return Track.UnlockedChannels & GetApplicableChannels(Track);
}

uint8 FAnimStitchManualEdits::GetActiveChannels(const FAnimStitchTrack& Track, double Time)
{
	constexpr double Eps = 1e-6;
	uint8 Mask = 0;
	for (const uint8 Bit : { AnimStitchChannel::Translation, AnimStitchChannel::Rotation, AnimStitchChannel::Scale })
	{
		double First = TNumericLimits<double>::Max(), Last = -TNumericLimits<double>::Max();
		for (const FAnimStitchTrackKey& K : Track.Keys)
		{
			if (!(K.Channels & Bit)) { continue; }
			First = FMath::Min(First, K.Time);
			Last = FMath::Max(Last, K.Time);
		}
		if (First > Last) { continue; } // this channel has no keys
		if (Track.Mode != EAnimStitchTrackMode::Absolute || (Time >= First - Eps && Time <= Last + Eps)) { Mask |= Bit; }
	}
	return Mask;
}

int32 FAnimStitchManualEdits::SetChannelKey(FAnimStitchTrack& Track, const FAnimStitchTrackKey& Key, uint8 Channels)
{
	Channels &= AnimStitchChannel::All;
	if (Channels == 0) { return INDEX_NONE; }
	auto CopyChannels = [&](FAnimStitchTrackKey& Dst)
	{
		if (Channels & AnimStitchChannel::Translation) { Dst.Translation = Key.Translation; }
		if (Channels & AnimStitchChannel::Rotation) { Dst.Rotation = Key.Rotation; }
		if (Channels & AnimStitchChannel::Scale) { Dst.Scale = Key.Scale; }
		Dst.Channels |= Channels;
	};
	const int32 Existing = FindKey(Track, Key.Time);
	if (Existing != INDEX_NONE)
	{
		CopyChannels(Track.Keys[Existing]);
		return Existing;
	}
	FAnimStitchTrackKey NewKey;
	NewKey.Time = Key.Time;
	NewKey.Pole = Key.Pole;
	NewKey.Weight = Key.Weight;
	NewKey.Channels = 0;
	CopyChannels(NewKey);
	int32 Index = 0;
	while (Index < Track.Keys.Num() && Track.Keys[Index].Time < Key.Time) { ++Index; }
	Track.Keys.Insert(NewKey, Index);
	return Index;
}

bool FAnimStitchManualEdits::IsActiveAt(const FAnimStitchTrack& Track, double Time)
{
	if (Track.Keys.Num() == 0) { return false; }
	if (Track.Mode == EAnimStitchTrackMode::Absolute && Track.Kind == EAnimStitchTrackKind::Bone)
	{
		return GetActiveChannels(Track, Time) != 0;
	}
	if (Track.Mode == EAnimStitchTrackMode::Absolute)
	{
		// An absolute track replaces the animation only across its keyed range; outside it the underlying animation plays untouched.
		constexpr double Eps = 1e-6;
		return Time >= Track.Keys[0].Time - Eps && Time <= Track.Keys.Last().Time + Eps;
	}
	return !IsIdentityKey(Sample(Track, Time));
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
	if (Track.Kind == EAnimStitchTrackKind::Bone)
	{
		// Each channel interpolates between its own keys; a channel without keys stays at identity.
		FAnimStitchTrackKey Out;
		Out.Time = Time;
		for (const uint8 Bit : { AnimStitchChannel::Translation, AnimStitchChannel::Rotation, AnimStitchChannel::Scale })
		{
			const FAnimStitchTrackKey* Prev = nullptr;
			const FAnimStitchTrackKey* Next = nullptr;
			for (const FAnimStitchTrackKey& K : Track.Keys)
			{
				if (!(K.Channels & Bit)) { continue; }
				if (K.Time <= Time) { Prev = &K; } else { Next = &K; break; }
			}
			const FAnimStitchTrackKey* A = Prev ? Prev : Next;
			const FAnimStitchTrackKey* B = Next ? Next : Prev;
			if (!A) { continue; }
			const double Span = B->Time - A->Time;
			const double Alpha = (A != B && Span > 1e-9) ? FMath::Clamp((Time - A->Time) / Span, 0.0, 1.0) : 0.0;
			if (Bit == AnimStitchChannel::Translation) { Out.Translation = FMath::Lerp(A->Translation, B->Translation, Alpha); }
			else if (Bit == AnimStitchChannel::Rotation) { Out.Rotation = FQuat::Slerp(A->Rotation, B->Rotation, Alpha).GetNormalized(); }
			else { Out.Scale = FMath::Lerp(A->Scale, B->Scale, Alpha); }
		}
		return Out;
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
				if (!Entry.Value->bKeepFeetPlanted || !IsPelvisBone(Entry.Value->Bone) || !IsActiveAt(*Entry.Value, Time)) { continue; }
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
		if (!IsActiveAt(Track, Time)) { continue; }
		const int32 Bone = Ref.FindBoneIndex(Track.Bone);
		const int32 Parent = Ref.GetParentIndex(Bone);
		const FTransform ParentCS = Parent != INDEX_NONE ? FAnimStitchFootLock::GetComponentSpace(Ref, Local, Parent) : FTransform::Identity;
		FTransform& L = Local[Bone];
		if (Track.Mode == EAnimStitchTrackMode::Absolute)
		{
			// Absolute: the key is the bone's full transform RELATIVE TO THE ROOT BONE (so it travels with root motion instead of staying at a world spot).
			// It replaces the blended result outright; the bone is re-expressed relative to its (already corrected) parent so children simply follow.
			// Per channel: only the channels keyed here replace the blend, the others keep what the animation produces.
			const uint8 Active = GetActiveChannels(Track, Time);
			FTransform KeyRel(Key.Rotation.GetNormalized(), Key.Translation, Key.Scale);
			if (Active != AnimStitchChannel::All)
			{
				KeyRel = FAnimStitchFootLock::GetComponentSpace(Ref, Local, Bone).GetRelativeTransform(Local[0]);
				if (Active & AnimStitchChannel::Translation) { KeyRel.SetTranslation(Key.Translation); }
				if (Active & AnimStitchChannel::Rotation) { KeyRel.SetRotation(Key.Rotation.GetNormalized()); }
				if (Active & AnimStitchChannel::Scale) { KeyRel.SetScale3D(Key.Scale); }
			}
			const FTransform Target = KeyRel * Local[0];
			const FTransform Relative = Target.GetRelativeTransform(ParentCS);
			if (!Track.bRotationOnly)
			{
				L.SetTranslation(Relative.GetTranslation());
				L.SetScale3D(Relative.GetScale3D());
			}
			L.SetRotation(Relative.GetRotation().GetNormalized());
			continue;
		}
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
		// The pole target comes from this limb's own pole lane when it has keys, else from the IK key (older timelines); none = keep the animated bend.
		FVector PoleTarget = Key.Pole;
		bool bHavePole = !Key.Pole.IsNearlyZero(1e-3);
		for (const FAnimStitchTrack& PoleTrack : Tracks)
		{
			if (PoleTrack.Kind == EAnimStitchTrackKind::IKPole && PoleTrack.Bone == Track.Bone && PoleTrack.Keys.Num() > 0)
			{
				PoleTarget = Sample(PoleTrack, Time).Translation;
				bHavePole = true;
				break;
			}
		}
		const FVector* Pole = bHavePole ? &PoleTarget : nullptr;
		FAnimStitchFootLock::SolveLimb(Ref, Local, Chain, Desired, Pole, Pole ? Weight : 0.0, &Rotation);
	}
}

