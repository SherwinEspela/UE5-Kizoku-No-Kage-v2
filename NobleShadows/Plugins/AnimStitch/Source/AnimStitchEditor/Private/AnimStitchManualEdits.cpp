#include "AnimStitchManualEdits.h"
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"

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

bool FAnimStitchManualEdits::IsRigBone(FName Bone)
{
	static const TCHAR* Excluded[] = { TEXT("twist"), TEXT("corrective"), TEXT("roll"), TEXT("helper"), TEXT("_ik"), TEXT("ik_"), TEXT("_end"), TEXT("jaw"), TEXT("eye"), TEXT("tongue"), TEXT("teeth"), TEXT("hair"), TEXT("brow"), TEXT("lip"), TEXT("cheek"), TEXT("nose"), TEXT("ear"), TEXT("facial"), TEXT("pole"), TEXT("attach"), TEXT("socket") };
	const FString Lower = Bone.ToString().ToLower();
	if (Lower.StartsWith(TEXT("ik")) && Lower.Len() > 2) { return false; }
	for (const TCHAR* Word : Excluded)
	{
		// A word counts only where it starts (not inside another word): "ear" is a face bone, "forearm" is not.
		for (int32 At = Lower.Find(Word, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0); At != INDEX_NONE; At = Lower.Find(Word, ESearchCase::CaseSensitive, ESearchDir::FromStart, At + 1))
		{
			if (Word[0] == TEXT('_') || At == 0 || !FChar::IsAlpha(Lower[At - 1])) { return false; }
		}
	}
	return true;
}

bool FAnimStitchManualEdits::IsControlBone(const USkeleton* Skeleton, FName Bone)
{
	if (!IsRigBone(Bone)) { return false; }
	const FString Name = Bone.ToString();
	if (Name.StartsWith(TEXT("VB "), ESearchCase::IgnoreCase)) { return false; } // virtual bones are named "VB <source>_<target>"
	if (!Skeleton) { return true; }
	for (const FVirtualBone& Virtual : Skeleton->GetVirtualBones()) { if (Virtual.VirtualBoneName == Bone) { return false; } }
	for (const USkeletalMeshSocket* Socket : Skeleton->Sockets) { if (Socket && Socket->SocketName == Bone) { return false; } }
	if (const USkeletalMesh* PreviewMesh = Skeleton->GetPreviewMesh())
	{
		for (const USkeletalMeshSocket* Socket : PreviewMesh->GetMeshOnlySocketList()) { if (Socket && Socket->SocketName == Bone) { return false; } }
	}
	return true;
}

int32 FAnimStitchManualEdits::ResolveParentTrack(const FReferenceSkeleton& Ref, const TArray<FAnimStitchTrack>& Tracks, int32 TrackIndex)
{
	if (!Tracks.IsValidIndex(TrackIndex) || Tracks[TrackIndex].Kind != EAnimStitchTrackKind::Bone) { return INDEX_NONE; }
	const int32 Bone = Ref.FindBoneIndex(Tracks[TrackIndex].Bone);
	if (Bone == INDEX_NONE) { return INDEX_NONE; }
	for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
	{
		const FName ParentName = Ref.GetBoneName(Parent);
		const int32 Found = Tracks.IndexOfByPredicate([ParentName](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::Bone && T.Bone == ParentName; });
		if (Found != INDEX_NONE) { return Found; }
	}
	return INDEX_NONE;
}

FName FAnimStitchManualEdits::FindLimbEffector(const FReferenceSkeleton& Ref, FName Bone)
{
	const int32 B = Ref.FindBoneIndex(Bone);
	if (B == INDEX_NONE) { return NAME_None; }
	auto IsEnd = [&Ref](int32 Index)
	{
		const FName Name = Ref.GetBoneName(Index);
		const FString Lower = Name.ToString().ToLower();
		if (Lower.Contains(TEXT("twist")) || Lower.Contains(TEXT("ik")) || Lower.Contains(TEXT("corrective")) || Lower.Contains(TEXT("helper"))) { return false; }
		return (Lower.Contains(TEXT("hand")) || Lower.Contains(TEXT("foot"))) && ChainFor(Ref, Name).IsValid();
	};
	if (IsEnd(B)) { return Ref.GetBoneName(B); }
	for (int32 Child = B + 1; Child < Ref.GetNum(); ++Child)
	{
		if (Ref.GetParentIndex(Child) != B) { continue; }
		if (IsEnd(Child)) { return Ref.GetBoneName(Child); }
		for (int32 Grand = Child + 1; Grand < Ref.GetNum(); ++Grand)
		{
			if (Ref.GetParentIndex(Grand) == Child && IsEnd(Grand)) { return Ref.GetBoneName(Grand); }
		}
	}
	return NAME_None;
}

bool FAnimStitchManualEdits::IsLimbIKAt(const FAnimStitchTrack& IKTrack, double Time)
{
	if (IKTrack.LimbSwitches.Num() == 0) { return true; }
	bool bIK = !IKTrack.LimbSwitches[0].bIK; // before the first switch the limb is in the mode the first switch leaves
	for (const FAnimStitchLimbSwitch& Switch : IKTrack.LimbSwitches)
	{
		if (Switch.Time <= Time + 1e-9) { bIK = Switch.bIK; } else { break; }
	}
	return bIK;
}

int32 FAnimStitchManualEdits::FindIKTrack(const TArray<FAnimStitchTrack>& Tracks, FName Effector)
{
	return Tracks.IndexOfByPredicate([Effector](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::IKLimb && T.Bone == Effector; });
}

bool FAnimStitchManualEdits::IsIdentityKey(const FAnimStitchTrackKey& Key)
{
	return Key.Translation.IsNearlyZero(1e-4) && Key.Rotation.Equals(FQuat::Identity, 1e-5) && Key.Scale.Equals(FVector::OneVector, 1e-5);
}

namespace
{
	/** IK keys are events: before the first key the weight eases in over this long. */
	constexpr double IKWeightRamp = 0.1;

	/**
	 * IK keys interpolate like every other track: across the FULL span between two keys the target (translation, rotation, pole) and the weight all use the SAME
	 * normalised position, starting at the first key and arriving exactly at the next. Every key carries a target of its own (see NormalizeIKKeys), so there is
	 * nothing to borrow from a neighbour. Only a key that still has no target at all (data that was never normalised) falls back to its neighbour's.
	 */
	bool HasIKTarget(const FAnimStitchTrackKey& K)
	{
		// A fully engaged key always has a target (even one at the origin); an IK-off or half-faded key with nothing stored has none.
		return K.Weight >= 0.999f || !(K.Translation.IsNearlyZero(1e-3) && K.Rotation.Equals(FQuat::Identity, 1e-5) && K.Pole.IsNearlyZero(1e-3));
	}

	FAnimStitchTrackKey SampleIK(const FAnimStitchTrack& Track, double Time)
	{
		const TArray<FAnimStitchTrackKey>& Keys = Track.Keys;
		int32 I = INDEX_NONE;
		for (int32 k = 0; k < Keys.Num() && Keys[k].Time <= Time; ++k) { I = k; }
		if (!Keys.IsValidIndex(I))
		{
			// Before the first key (unchanged): IK eases in over the last IKWeightRamp seconds.
			const FAnimStitchTrackKey& First = Keys[0];
			FAnimStitchTrackKey Out = First;
			const double Ramp = FMath::Max(FMath::Min(IKWeightRamp, First.Time), 1e-6);
			const double RampStart = First.Time - Ramp;
			Out.Weight = Time > RampStart ? FMath::Lerp(0.f, First.Weight, float(FMath::Clamp((Time - RampStart) / Ramp, 0.0, 1.0))) : 0.f;
			Out.Time = Time;
			return Out;
		}
		const FAnimStitchTrackKey& K = Keys[I];
		if (!Keys.IsValidIndex(I + 1))
		{
			FAnimStitchTrackKey Out = K; // after the last key (unchanged): hold
			if (!HasIKTarget(K) && I > 0 && HasIKTarget(Keys[I - 1]))
			{
				Out.Translation = Keys[I - 1].Translation;
				Out.Rotation = Keys[I - 1].Rotation;
				Out.Pole = Keys[I - 1].Pole;
			}
			Out.Time = Time;
			return Out;
		}
		const FAnimStitchTrackKey& Next = Keys[I + 1];
		const double Span = Next.Time - K.Time;
		const double T = Span > 1e-9 ? FMath::Clamp((Time - K.Time) / Span, 0.0, 1.0) : 0.0;
		// A key with no target at all (never normalised) borrows its neighbour's, so the span still interpolates between two real targets.
		const FAnimStitchTrackKey& From = (!HasIKTarget(K) && HasIKTarget(Next)) ? Next : K;
		const FAnimStitchTrackKey& To = (!HasIKTarget(Next) && HasIKTarget(K)) ? K : Next;
		FAnimStitchTrackKey Out = K;
		Out.Translation = FMath::Lerp(From.Translation, To.Translation, T);
		Out.Rotation = FQuat::Slerp(From.Rotation, To.Rotation, T).GetNormalized();
		Out.Pole = FMath::Lerp(From.Pole, To.Pole, T);
		Out.Weight = FMath::Lerp(K.Weight, Next.Weight, float(T));
		Out.Time = Time;
		return Out;
	}
}

bool FAnimStitchManualEdits::NormalizeIKKeys(FAnimStitchTrack& Track)
{
	if (Track.Kind != EAnimStitchTrackKind::IKLimb) { return false; }
	bool bChanged = false;
	TArray<FAnimStitchTrackKey>& Keys = Track.Keys;
	for (int32 k = 0; k < Keys.Num(); ++k)
	{
		if (HasIKTarget(Keys[k])) { continue; }
		// Older builds stored "IK off" anchors and half-faded keys with no target at all. Give them the target of the nearest key that has one, so every key is a full
		// (target, pole, weight) value and the span between any two keys interpolates all of it together.
		int32 Best = INDEX_NONE;
		for (int32 d = 1; d < Keys.Num() && Best == INDEX_NONE; ++d)
		{
			const bool bNext = Keys.IsValidIndex(k + d) && HasIKTarget(Keys[k + d]);
			const bool bPrev = Keys.IsValidIndex(k - d) && HasIKTarget(Keys[k - d]);
			if (bNext && bPrev) { Best = (Keys[k + d].Time - Keys[k].Time) <= (Keys[k].Time - Keys[k - d].Time) ? k + d : k - d; }
			else if (bNext) { Best = k + d; }
			else if (bPrev) { Best = k - d; }
		}
		if (Best == INDEX_NONE) { continue; }
		Keys[k].Translation = Keys[Best].Translation;
		Keys[k].Rotation = Keys[Best].Rotation;
		Keys[k].Pole = Keys[Best].Pole;
		bChanged = true;
	}
	return bChanged;
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
		// An absolute track owns its bone: a channel with any key is active on the whole timeline. Between keys it interpolates, before the first / after the last it holds that key.
		Mask |= Bit;
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
		FAnimStitchTrackKey Out = SampleIK(Track, Time);
		if (!IsLimbIKAt(Track, Time)) { Out.Weight = 0.f; } // FK mode: the IK is off, the FK joints drive the limb
		return Out;
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
		if (bMatch && !bBad && !Lower.StartsWith(TEXT("vb ")) && BoneDepth(RefSkeleton, i) < 9) { Result.Add(RefSkeleton.GetBoneName(i)); }
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

void FAnimStitchManualEdits::Apply(const FReferenceSkeleton& Ref, const TArray<FAnimStitchTrack>& Tracks, double Time, TArray<FTransform>& Local, const TArray<FName>* PlantFootBones)
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
		for (int32 f = 0; f < PlantFootBones->Num(); ++f)
		{
			const FAnimStitchFootChain Chain = ChainFor(Ref, (*PlantFootBones)[f]);
			if (!Chain.IsValid()) { continue; }
			bool bIKOwned = false;
			for (const FAnimStitchTrack& T : Tracks)
			{
				if (T.Kind == EAnimStitchTrackKind::IKLimb && T.Bone == (*PlantFootBones)[f] && T.Keys.Num() > 0 && Sample(T, Time).Weight > 1e-3f) { bIKOwned = true; }
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

	// Control hierarchy: an Absolute control places its bone at a fixed spot, so a parented one also follows what its parent control did to the parent bone
	// (the parent bone's change from the blended pose to its corrected pose, which already includes everything above it).
	// The pose before any control touches it: additive deltas are expressed in ITS axes (see below), and parented Absolute controls follow their parent's change from it.
	TArray<FTransform> Original;
	if (BoneTracks.Num() > 0) { Original = Local; }
	// The control hierarchy is derived from the skeleton, not stored: a control's parent is the control on its nearest ancestor bone.
	TArray<uint8> HasControl;
	HasControl.Init(0, Ref.GetNum());
	for (const TPair<int32, const FAnimStitchTrack*>& Entry : BoneTracks) { HasControl[Ref.FindBoneIndex(Entry.Value->Bone)] = 1; }

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
			// The channels this key does not hold stay as the animation produces them (taken from the pose before any control), so the parent's motion below carries them too.
			FTransform KeyRel(Key.Rotation.GetNormalized(), Key.Translation, Key.Scale);
			if (Active != AnimStitchChannel::All)
			{
				KeyRel = FAnimStitchFootLock::GetComponentSpace(Ref, Original, Bone).GetRelativeTransform(Original[0]);
				if (Active & AnimStitchChannel::Translation) { KeyRel.SetTranslation(Key.Translation); }
				if (Active & AnimStitchChannel::Rotation) { KeyRel.SetRotation(Key.Rotation.GetNormalized()); }
				if (Active & AnimStitchChannel::Scale) { KeyRel.SetScale3D(Key.Scale); }
			}
			FTransform Target = KeyRel * Local[0];
			int32 ParentControlBone = INDEX_NONE;
			for (int32 Ancestor = Parent; Ancestor != INDEX_NONE; Ancestor = Ref.GetParentIndex(Ancestor))
			{
				if (HasControl[Ancestor]) { ParentControlBone = Ancestor; break; }
			}
			if (Original.Num() > 0 && ParentControlBone != INDEX_NONE)
			{
				const FTransform ParentBefore = FAnimStitchFootLock::GetComponentSpace(Ref, Original, ParentControlBone);
				const FTransform ParentAfter = FAnimStitchFootLock::GetComponentSpace(Ref, Local, ParentControlBone);
				const FTransform ParentMove = ParentBefore.Inverse() * ParentAfter; // the parent bone's own motion, in its frame
				if (!ParentMove.Equals(FTransform::Identity, 1e-4f)) { Target = Target * ParentMove; }
			}
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
			// Additive deltas are expressed in the component axes of the ANIMATION (the pose before the controls), not of the parent as other controls moved it:
			// rotating a parent control then carries this bone rigidly (its own local rotation / offset stays exactly as keyed) instead of re-aiming its delta.
			L.SetTranslation(L.GetTranslation() + (Parent != INDEX_NONE ? FAnimStitchFootLock::GetComponentSpace(Ref, Original, Parent) : FTransform::Identity).InverseTransformVector(Key.Translation));
			L.SetScale3D(L.GetScale3D() * Key.Scale);
		}
		if (!Key.Rotation.IsIdentity(1e-6))
		{
			// The delta is a component-space rotation about the bone: conjugate it into the parent's space.
			const FQuat ParentRot = Parent != INDEX_NONE ? FAnimStitchFootLock::GetComponentSpace(Ref, Original, Parent).GetRotation() : FQuat::Identity;
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

void FAnimStitchManualEdits::ConvertTracksToAbsolute(const FReferenceSkeleton& Ref, TArray<FAnimStitchTrack>& Tracks, const TArray<TArray<FTransform>>& BlendPoses, double Rate, double TotalLength, const TArray<FName>* PlantFootBones, const TArray<int32>* OnlyTracks, bool bEveryFrame)
{
	(void)PlantFootBones; // the plant / IK stages run after the bone stage that is captured here, so they are not part of the keys
	if (BlendPoses.Num() == 0 || Rate <= 0.0) { return; }

	TArray<FAnimStitchTrack> BoneOnly;
	for (const FAnimStitchTrack& Track : Tracks) { if (Track.Kind == EAnimStitchTrackKind::Bone) { BoneOnly.Add(Track); } }

	struct FConvert { int32 Track; int32 Bone; int32 ParentBone; TArray<FAnimStitchTrackKey> Keys; };
	TArray<FConvert> Converts;
	for (int32 i = 0; i < Tracks.Num(); ++i)
	{
		if (Tracks[i].Kind != EAnimStitchTrackKind::Bone || Tracks[i].Mode != EAnimStitchTrackMode::Additive) { continue; }
		if (OnlyTracks && !OnlyTracks->Contains(i)) { continue; }
		const int32 Bone = Ref.FindBoneIndex(Tracks[i].Bone);
		if (Bone == INDEX_NONE) { continue; }
		const int32 ParentTrack = ResolveParentTrack(Ref, Tracks, i);
		Converts.Add({ i, Bone, ParentTrack != INDEX_NONE ? Ref.FindBoneIndex(Tracks[ParentTrack].Bone) : INDEX_NONE, {} });
	}
	if (Converts.Num() == 0) { return; }

	const int32 NumFrames = BlendPoses.Num();
	for (int32 f = 0; f < NumFrames; ++f)
	{
		const double Time = f == NumFrames - 1 ? TotalLength : f / Rate;
		const TArray<FTransform>& Orig = BlendPoses[f];
		TArray<FTransform> Stage = Orig;
		Apply(Ref, BoneOnly, Time, Stage, nullptr);
		for (FConvert& C : Converts)
		{
			FTransform ParentMove = FTransform::Identity;
			if (C.ParentBone != INDEX_NONE)
			{
				ParentMove = FAnimStitchFootLock::GetComponentSpace(Ref, Orig, C.ParentBone).Inverse() * FAnimStitchFootLock::GetComponentSpace(Ref, Stage, C.ParentBone);
			}
			const FTransform Wanted = FAnimStitchFootLock::GetComponentSpace(Ref, Stage, C.Bone) * ParentMove.Inverse();
			const FTransform Basis = C.Bone == 0 ? Orig[0] : Stage[0]; // the root frame the key is relative to, as it is when Apply reaches this track
			const FTransform Rel = Wanted.GetRelativeTransform(Basis);
			FAnimStitchTrackKey Key;
			Key.Time = Time;
			Key.Translation = Rel.GetTranslation();
			Key.Rotation = Rel.GetRotation().GetNormalized();
			Key.Scale = Rel.GetScale3D();
			C.Keys.Add(Key);
		}
	}
	for (FConvert& C : Converts)
	{
		FAnimStitchTrack& Track = Tracks[C.Track];
		if (!bEveryFrame)
		{
			// Existing keyframes only: Absolute keys at the frames the track already had keys on (plus the first / last frame so it holds).
			TSet<int32> Frames;
			Frames.Add(0);
			Frames.Add(C.Keys.Num() - 1);
			for (const FAnimStitchTrackKey& Old : Track.Keys) { Frames.Add(FMath::Clamp(FMath::RoundToInt32(Old.Time * Rate), 0, C.Keys.Num() - 1)); }
			TArray<FAnimStitchTrackKey> Thinned;
			for (int32 f = 0; f < C.Keys.Num(); ++f) { if (Frames.Contains(f)) { Thinned.Add(C.Keys[f]); } }
			C.Keys = MoveTemp(Thinned);
		}
		Track.Mode = EAnimStitchTrackMode::Absolute;
		Track.Keys = MoveTemp(C.Keys);
	}
}