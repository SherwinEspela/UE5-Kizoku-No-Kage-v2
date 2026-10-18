#include "AnimStitchBaker.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "ReferenceSkeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

UAnimSequence* FAnimStitchBaker::Bake(const FAnimStitchTimeline& Timeline, const FString& PackagePath, FFrameRate FrameRate, bool bSavePackage, FString& OutError)
{
	FAnimStitchEvaluator Evaluator;
	if (!Evaluator.Initialize(Timeline, OutError))
	{
		return nullptr;
	}
	if (!FPackageName::IsValidLongPackageName(PackagePath))
	{
		OutError = FText::Format(LOCTEXT("ErrPath", "'{0}' is not a valid asset path."), FText::FromString(PackagePath)).ToString();
		return nullptr;
	}

	if (FPackageName::DoesPackageExist(PackagePath) || FindPackage(nullptr, *PackagePath))
	{
		OutError = FText::Format(LOCTEXT("ErrExists", "An asset already exists at '{0}'. Choose a new name."), FText::FromString(PackagePath)).ToString();
		return nullptr;
	}

	const UAnimSequence* First = Timeline.Clips[0].Sequence.Get();
	if (!FrameRate.IsValid())
	{
		FrameRate = First->GetDataModel()->GetFrameRate();
	}

	const double Length = Timeline.GetTotalLength();
	const int32 NumFrames = FMath::Max(1, FMath::RoundToInt(Length * FrameRate.AsDecimal()));
	const int32 NumKeys = NumFrames + 1;
	const int32 NumBones = Evaluator.GetNumBones();
	const FReferenceSkeleton& RefSkel = Evaluator.GetSkeleton()->GetReferenceSkeleton();

	// Sample the shared evaluator once per output frame.
	TArray<TArray<FVector3f>> Pos;
	TArray<TArray<FQuat4f>> Rot;
	TArray<TArray<FVector3f>> Scl;
	Pos.SetNum(NumBones);
	Rot.SetNum(NumBones);
	Scl.SetNum(NumBones);
	TArray<FTransform> Pose;
	for (int32 Key = 0; Key < NumKeys; ++Key)
	{
		Evaluator.EvaluatePose(Key / FrameRate.AsDecimal(), Pose);
		for (int32 b = 0; b < NumBones; ++b)
		{
			Pos[b].Add(FVector3f(Pose[b].GetTranslation()));
			Rot[b].Add(FQuat4f(Pose[b].GetRotation().GetNormalized()));
			Scl[b].Add(FVector3f(Pose[b].GetScale3D()));
		}
	}

	UPackage* Package = CreatePackage(*PackagePath);
	const FString AssetName = FPackageName::GetShortName(PackagePath);
	UAnimSequence* Seq = NewObject<UAnimSequence>(Package, *AssetName, RF_Public | RF_Standalone);
	Seq->SetSkeleton(Evaluator.GetSkeleton());

	IAnimationDataController& Controller = Seq->GetController();
	Controller.OpenBracket(LOCTEXT("BakeBracket", "AnimStitch Bake"), false);
	Controller.InitializeModel();
	Controller.SetFrameRate(FrameRate, false);
	Controller.SetNumberOfFrames(FFrameNumber(NumFrames), false);
	for (int32 b = 0; b < NumBones; ++b)
	{
		const FName BoneName = RefSkel.GetBoneName(b);
		Controller.AddBoneCurve(BoneName, false);
		Controller.SetBoneTrackKeys(BoneName, Pos[b], Rot[b], Scl[b], false);
	}
	Controller.NotifyPopulated();
	Controller.CloseBracket(false);

	// Root motion follows the authority clip.
	const UAnimSequence* Auth = Timeline.Clips[FMath::Clamp(Timeline.RootAuthorityIndex, 0, Timeline.Clips.Num() - 1)].Sequence.Get();
	Seq->bEnableRootMotion = Auth->bEnableRootMotion;
	Seq->RootMotionRootLock = Auth->RootMotionRootLock;
	Seq->bForceRootLock = Auth->bForceRootLock;

	Seq->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Seq);

	if (bSavePackage)
	{
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString File = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Package, Seq, *File, Args))
		{
			OutError = LOCTEXT("ErrSave", "Failed to save the baked sequence.").ToString();
			return nullptr;
		}
	}
	return Seq;
}

#undef LOCTEXT_NAMESPACE
