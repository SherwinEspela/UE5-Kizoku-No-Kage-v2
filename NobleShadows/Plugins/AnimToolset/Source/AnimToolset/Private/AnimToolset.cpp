#include "AnimToolset.h"

#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/Skeleton.h"
#include "Dom/JsonObject.h"
#include "ReferenceSkeleton.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString ToJsonString(const TSharedRef<FJsonObject>& Obj)
	{
		FString Out;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Obj, Writer);
		return Out;
	}

	FString ErrorJson(const FString& Message)
	{
		TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetBoolField(TEXT("ok"), false);
		Obj->SetStringField(TEXT("error"), Message);
		return ToJsonString(Obj);
	}

	TSharedRef<FJsonValue> Vec3(const FVector& V)
	{
		TArray<TSharedPtr<FJsonValue>> A;
		A.Add(MakeShared<FJsonValueNumber>(V.X));
		A.Add(MakeShared<FJsonValueNumber>(V.Y));
		A.Add(MakeShared<FJsonValueNumber>(V.Z));
		return MakeShared<FJsonValueArray>(A);
	}

	/** Local-space transform of a bone at a frame: animated track if present, else the reference pose. */
	FTransform LocalBoneAtFrame(const IAnimationDataModel* Model, const FReferenceSkeleton& RefSkel, int32 BoneIndex, int32 Frame)
	{
		const FName BoneName = RefSkel.GetBoneName(BoneIndex);
		if (Model->IsValidBoneTrackName(BoneName))
		{
			return Model->EvaluateBoneTrackTransform(BoneName, FFrameTime(FFrameNumber(Frame)), EAnimInterpolationType::Linear);
		}
		return RefSkel.GetRefBonePose()[BoneIndex];
	}

	/** Component-space transform of a bone at a frame, chaining local transforms from the root. */
	FTransform ComponentBoneAtFrame(const IAnimationDataModel* Model, const FReferenceSkeleton& RefSkel, int32 BoneIndex, int32 Frame)
	{
		FTransform Result = FTransform::Identity;
		int32 Index = BoneIndex;
		while (Index != INDEX_NONE)
		{
			Result = Result * LocalBoneAtFrame(Model, RefSkel, Index, Frame);
			Index = RefSkel.GetParentIndex(Index);
		}
		return Result;
	}

	bool GetModelAndSkel(UAnimSequenceBase* Seq, const IAnimationDataModel*& OutModel, const FReferenceSkeleton*& OutSkel, FString& OutError)
	{
		if (!Seq)
		{
			OutError = TEXT("Sequence is null");
			return false;
		}
		OutModel = Seq->GetDataModelInterface().GetInterface();
		if (!OutModel)
		{
			OutError = TEXT("Sequence has no animation data model");
			return false;
		}
		USkeleton* Skeleton = Seq->GetSkeleton();
		if (!Skeleton)
		{
			OutError = TEXT("Sequence has no skeleton");
			return false;
		}
		OutSkel = &Skeleton->GetReferenceSkeleton();
		return true;
	}
}

FString UAnimToolset::GetAnimInfo(UAnimSequenceBase* Sequence)
{
	const IAnimationDataModel* Model = nullptr;
	const FReferenceSkeleton* RefSkel = nullptr;
	FString Error;
	if (!GetModelAndSkel(Sequence, Model, RefSkel, Error))
	{
		return ErrorJson(Error);
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetStringField(TEXT("path"), Sequence->GetPathName());
	Obj->SetNumberField(TEXT("lengthSeconds"), Model->GetPlayLength());
	const FFrameRate Rate = Model->GetFrameRate();
	Obj->SetNumberField(TEXT("fps"), Rate.AsDecimal());
	Obj->SetNumberField(TEXT("numFrames"), Model->GetNumberOfFrames());
	Obj->SetNumberField(TEXT("numKeys"), Model->GetNumberOfKeys());
	Obj->SetNumberField(TEXT("rateScale"), Sequence->RateScale);
	Obj->SetBoolField(TEXT("loop"), Sequence->bLoop);
	Obj->SetNumberField(TEXT("numNotifies"), Sequence->Notifies.Num());
	Obj->SetStringField(TEXT("skeleton"), Sequence->GetSkeleton()->GetPathName());
	Obj->SetNumberField(TEXT("numSkeletonBones"), RefSkel->GetNum());

	if (const UAnimSequence* AnimSeq = Cast<UAnimSequence>(Sequence))
	{
		Obj->SetBoolField(TEXT("rootMotionEnabled"), AnimSeq->bEnableRootMotion);
		Obj->SetNumberField(TEXT("additiveAnimType"), static_cast<int32>(AnimSeq->AdditiveAnimType));
	}
	return ToJsonString(Obj);
}

FString UAnimToolset::ListNotifies(UAnimSequenceBase* Sequence)
{
	if (!Sequence)
	{
		return ErrorJson(TEXT("Sequence is null"));
	}

	double Fps = 30.0;
	if (const IAnimationDataModel* Model = Sequence->GetDataModelInterface().GetInterface())
	{
		Fps = Model->GetFrameRate().AsDecimal();
	}

	TArray<TSharedPtr<FJsonValue>> Items;
	for (int32 i = 0; i < Sequence->Notifies.Num(); ++i)
	{
		const FAnimNotifyEvent& Ev = Sequence->Notifies[i];
		TSharedRef<FJsonObject> N = MakeShared<FJsonObject>();
		N->SetNumberField(TEXT("index"), i);
		N->SetStringField(TEXT("name"), Ev.NotifyName.ToString());
		const double Time = Ev.GetTriggerTime();
		N->SetNumberField(TEXT("timeSeconds"), Time);
		N->SetNumberField(TEXT("frame"), Time * Fps);
		N->SetNumberField(TEXT("duration"), Ev.GetDuration());
		N->SetNumberField(TEXT("trackIndex"), Ev.TrackIndex);
		N->SetStringField(TEXT("notifyClass"), Ev.Notify ? Ev.Notify->GetClass()->GetPathName() : FString());
		N->SetStringField(TEXT("notifyStateClass"), Ev.NotifyStateClass ? Ev.NotifyStateClass->GetClass()->GetPathName() : FString());
		Items.Add(MakeShared<FJsonValueObject>(N));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetNumberField(TEXT("fps"), Fps);
	Obj->SetNumberField(TEXT("count"), Items.Num());
	Obj->SetArrayField(TEXT("notifies"), Items);
	return ToJsonString(Obj);
}

FString UAnimToolset::ListBones(UAnimSequence* Sequence)
{
	const IAnimationDataModel* Model = nullptr;
	const FReferenceSkeleton* RefSkel = nullptr;
	FString Error;
	if (!GetModelAndSkel(Sequence, Model, RefSkel, Error))
	{
		return ErrorJson(Error);
	}

	TArray<TSharedPtr<FJsonValue>> Items;
	for (int32 i = 0; i < RefSkel->GetNum(); ++i)
	{
		const FName Name = RefSkel->GetBoneName(i);
		TSharedRef<FJsonObject> B = MakeShared<FJsonObject>();
		B->SetNumberField(TEXT("index"), i);
		B->SetStringField(TEXT("name"), Name.ToString());
		B->SetNumberField(TEXT("parent"), RefSkel->GetParentIndex(i));
		B->SetBoolField(TEXT("animated"), Model->IsValidBoneTrackName(Name));
		Items.Add(MakeShared<FJsonValueObject>(B));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetNumberField(TEXT("count"), Items.Num());
	Obj->SetArrayField(TEXT("bones"), Items);
	return ToJsonString(Obj);
}

FString UAnimToolset::GetBoneTrajectory(UAnimSequence* Sequence, const FString& BoneName, int32 StartFrame, int32 EndFrame)
{
	const IAnimationDataModel* Model = nullptr;
	const FReferenceSkeleton* RefSkel = nullptr;
	FString Error;
	if (!GetModelAndSkel(Sequence, Model, RefSkel, Error))
	{
		return ErrorJson(Error);
	}

	const int32 BoneIndex = RefSkel->FindBoneIndex(FName(*BoneName));
	if (BoneIndex == INDEX_NONE)
	{
		return ErrorJson(FString::Printf(TEXT("Bone '%s' not found in skeleton"), *BoneName));
	}

	const int32 LastFrame = FMath::Max(0, Model->GetNumberOfFrames());
	const int32 First = FMath::Clamp(StartFrame, 0, LastFrame);
	const int32 Last = EndFrame < 0 ? LastFrame : FMath::Clamp(EndFrame, First, LastFrame);

	const double Fps = Model->GetFrameRate().AsDecimal();
	TArray<TSharedPtr<FJsonValue>> Positions;
	for (int32 Frame = First; Frame <= Last; ++Frame)
	{
		Positions.Add(Vec3(ComponentBoneAtFrame(Model, *RefSkel, BoneIndex, Frame).GetLocation()));
	}

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetStringField(TEXT("bone"), BoneName);
	Obj->SetNumberField(TEXT("fps"), Fps);
	Obj->SetNumberField(TEXT("startFrame"), First);
	Obj->SetNumberField(TEXT("endFrame"), Last);
	Obj->SetArrayField(TEXT("positions"), Positions);
	return ToJsonString(Obj);
}

FString UAnimToolset::AddNotify(UAnimSequenceBase* Sequence, TSubclassOf<UAnimNotify> NotifyClass, float TimeSeconds, int32 TrackIndex)
{
	if (!Sequence)
	{
		return ErrorJson(TEXT("Sequence is null"));
	}
	if (!NotifyClass)
	{
		return ErrorJson(TEXT("NotifyClass is null"));
	}
	const float Length = Sequence->GetPlayLength();
	if (TimeSeconds < 0.f || TimeSeconds > Length)
	{
		return ErrorJson(FString::Printf(TEXT("TimeSeconds %f outside 0..%f"), TimeSeconds, Length));
	}
	for (const FAnimNotifyEvent& Existing : Sequence->Notifies)
	{
		if (Existing.Notify && Existing.Notify->GetClass() == NotifyClass && FMath::IsNearlyEqual(Existing.GetTriggerTime(), TimeSeconds, 0.001f))
		{
			return ErrorJson(TEXT("A notify of this class already exists at this time"));
		}
	}

	Sequence->Modify();
	const int32 NewIndex = Sequence->Notifies.AddDefaulted();
	FAnimNotifyEvent& Ev = Sequence->Notifies[NewIndex];
	Ev.Notify = NewObject<UAnimNotify>(Sequence, NotifyClass, NAME_None, RF_Transactional);
	Ev.NotifyName = FName(*Ev.Notify->GetNotifyName());
	Ev.TrackIndex = TrackIndex;
	Ev.Link(Sequence, TimeSeconds);
	Ev.TriggerTimeOffset = GetTriggerTimeOffsetForType(Sequence->CalculateOffsetForNotify(TimeSeconds));
	Sequence->SortNotifies();
	Sequence->RefreshCacheData();
	Sequence->MarkPackageDirty();

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetNumberField(TEXT("timeSeconds"), TimeSeconds);
	Obj->SetNumberField(TEXT("numNotifies"), Sequence->Notifies.Num());
	return ToJsonString(Obj);
}

FString UAnimToolset::RemoveNotify(UAnimSequenceBase* Sequence, int32 NotifyIndex)
{
	if (!Sequence)
	{
		return ErrorJson(TEXT("Sequence is null"));
	}
	if (!Sequence->Notifies.IsValidIndex(NotifyIndex))
	{
		return ErrorJson(TEXT("NotifyIndex out of range"));
	}
	Sequence->Modify();
	Sequence->Notifies.RemoveAt(NotifyIndex);
	Sequence->RefreshCacheData();
	Sequence->MarkPackageDirty();

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetNumberField(TEXT("numNotifies"), Sequence->Notifies.Num());
	return ToJsonString(Obj);
}

FString UAnimToolset::GetRootMotionSummary(UAnimSequence* Sequence)
{
	const IAnimationDataModel* Model = nullptr;
	const FReferenceSkeleton* RefSkel = nullptr;
	FString Error;
	if (!GetModelAndSkel(Sequence, Model, RefSkel, Error))
	{
		return ErrorJson(Error);
	}
	if (RefSkel->GetNum() == 0)
	{
		return ErrorJson(TEXT("Skeleton has no bones"));
	}

	const int32 LastFrame = FMath::Max(0, Model->GetNumberOfFrames());
	const double Length = Model->GetPlayLength();
	const FVector Start = LocalBoneAtFrame(Model, *RefSkel, 0, 0).GetLocation();
	const FVector End = LocalBoneAtFrame(Model, *RefSkel, 0, LastFrame).GetLocation();

	double PathLength = 0.0;
	FVector Prev = Start;
	for (int32 Frame = 1; Frame <= LastFrame; ++Frame)
	{
		const FVector P = LocalBoneAtFrame(Model, *RefSkel, 0, Frame).GetLocation();
		PathLength += FVector::Dist(Prev, P);
		Prev = P;
	}

	const FVector Delta = End - Start;
	const double Horizontal = FVector(Delta.X, Delta.Y, 0.0).Size();

	TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetBoolField(TEXT("ok"), true);
	Obj->SetStringField(TEXT("rootBone"), RefSkel->GetBoneName(0).ToString());
	Obj->SetBoolField(TEXT("rootMotionEnabled"), Sequence->bEnableRootMotion);
	Obj->SetNumberField(TEXT("lengthSeconds"), Length);
	Obj->SetField(TEXT("startLocation"), Vec3(Start));
	Obj->SetField(TEXT("endLocation"), Vec3(End));
	Obj->SetField(TEXT("delta"), Vec3(Delta));
	Obj->SetNumberField(TEXT("straightDistance"), Delta.Size());
	Obj->SetNumberField(TEXT("horizontalDistance"), Horizontal);
	Obj->SetNumberField(TEXT("pathLength"), PathLength);
	Obj->SetNumberField(TEXT("avgSpeed"), Length > 0.0 ? Delta.Size() / Length : 0.0);
	Obj->SetNumberField(TEXT("avgHorizontalSpeed"), Length > 0.0 ? Horizontal / Length : 0.0);
	return ToJsonString(Obj);
}
