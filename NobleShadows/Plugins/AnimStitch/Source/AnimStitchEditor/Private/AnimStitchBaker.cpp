#include "AnimStitchBaker.h"
#include "AnimStitchFootLock.h"
#include "AnimStitchManualEdits.h"
#include "AnimStitchProvenance.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AnimStitchSettings.h"
#include "ReferenceSkeleton.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void FAnimStitchBaker::NotifyBakeComplete(UAnimSequence* Baked)
{
	if (!Baked || !FSlateApplication::IsInitialized()) { return; }
	const TWeakObjectPtr<UAnimSequence> WeakBaked = Baked;
	const FString Name = Baked->GetName();
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakBaked, Name](float)
	{
		FNotificationInfo Info(FText::Format(NSLOCTEXT("AnimStitch", "BakeDone", "Baked {0}"), FText::FromString(Name)));
		Info.ExpireDuration = 10.f;
		Info.bUseSuccessFailIcons = true;
		Info.HyperlinkText = NSLOCTEXT("AnimStitch", "ShowInBrowser", "Show in Content Browser");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([WeakBaked]()
		{
			if (GEditor && WeakBaked.IsValid())
			{
				TArray<UObject*> Objects = { WeakBaked.Get() };
				GEditor->SyncBrowserToObjects(Objects);
			}
		});
		if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info)) { Item->SetCompletionState(SNotificationItem::CS_Success); }
		return false;
	}), 0.2f);
}

bool FAnimStitchBaker::AssetExists(const FString& PackagePath)
{
	return FPackageName::DoesPackageExist(PackagePath) || FindPackage(nullptr, *PackagePath) != nullptr;
}

FSimpleMulticastDelegate& FAnimStitchBaker::OnAssetBaked()
{
	static FSimpleMulticastDelegate Delegate;
	return Delegate;
}

UAnimSequence* FAnimStitchBaker::Bake(const FAnimStitchTimeline& Timeline, const FString& PackagePath, const FAnimStitchBakeOptions& Options, FString& OutError)
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

	const FString AssetName = FPackageName::GetShortName(PackagePath);
	UAnimSequence* Existing = nullptr;
	if (AssetExists(PackagePath))
	{
		if (!Options.bOverwriteExisting)
		{
			OutError = FText::Format(LOCTEXT("ErrExists", "An asset already exists at '{0}'. Choose a new name or allow overwriting."), FText::FromString(PackagePath)).ToString();
			return nullptr;
		}
		Existing = LoadObject<UAnimSequence>(nullptr, *(PackagePath + TEXT(".") + AssetName));
		if (!Existing)
		{
			OutError = FText::Format(LOCTEXT("ErrNotSequence", "'{0}' exists but is not an animation sequence, so it can't be overwritten."), FText::FromString(PackagePath)).ToString();
			return nullptr;
		}
		if (Existing->GetSkeleton() != Evaluator.GetSkeleton())
		{
			OutError = LOCTEXT("ErrOverwriteSkeleton", "The existing asset uses a different skeleton, so it can't be overwritten.").ToString();
			return nullptr;
		}
	}

	const UAnimSequence* First = Timeline.Clips[0].Sequence.Get();
	FFrameRate FrameRate = Options.FrameRate;
	if (!(FrameRate.AsDecimal() > 0.0))
	{
		FrameRate = First->GetDataModel()->GetFrameRate();
	}

	// Some assets report absurd source rates (tens of thousands of fps), which would bake millions of keys.
	constexpr double MaxSourceRate = 120.0;
	if (!(Options.FrameRate.AsDecimal() > 0.0) && FrameRate.AsDecimal() > MaxSourceRate)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimStitch: source frame rate %.0f fps is not usable for baking; using 30 fps instead."), FrameRate.AsDecimal());
		FrameRate = FFrameRate(30, 1);
	}

const double Length = Timeline.GetTotalLength();
	// The whole timeline is always evaluated (foot lock, prediction and manual edits work on absolute frames); a partial range only picks which frames are written.
	const int32 FullFrames = FMath::Max(1, FMath::RoundToInt(Length * FrameRate.AsDecimal()));
	int32 FirstFrame = 0, LastFrame = FullFrames;
	if (Options.bPartialRange)
	{
		FirstFrame = FMath::Clamp(FMath::RoundToInt(Options.RangeStart * FrameRate.AsDecimal()), 0, FullFrames - 1);
		LastFrame = FMath::Clamp(FMath::RoundToInt(Options.RangeEnd * FrameRate.AsDecimal()), FirstFrame + 1, FullFrames);
	}
	const int32 NumFrames = LastFrame - FirstFrame;
	const int32 NumKeys = NumFrames + 1;
	const int32 NumBones = Evaluator.GetNumBones();
	const FReferenceSkeleton& RefSkel = Evaluator.GetSkeleton()->GetReferenceSkeleton();

	// Only bones some clip animates get a track; the rest stay at the skeleton's reference pose.
	TArray<int32> TrackBones;
	for (int32 b = 0; b < NumBones; ++b)
	{
		if (b == 0 || Evaluator.IsBoneAnimated(b))
		{
			TrackBones.Add(b);
		}
	}
	// Foot lock rewrites the leg bones, so they need tracks even if no source animates them.
	const bool bFootLock = Timeline.FootLock.bEnabled;
	const bool bPredict = Timeline.bPredictFeet;
	const bool bManual = Timeline.Tracks.Num() > 0;
	const bool bKeepPoses = bFootLock || bPredict || bManual;
	// Manual corrections change these bones even if no source animates them.
	for (const FAnimStitchTrack& Track : Timeline.Tracks)
	{
		const int32 Bone = RefSkel.FindBoneIndex(Track.Bone);
		if (Bone == INDEX_NONE) { continue; }
		if (Track.Kind == EAnimStitchTrackKind::Bone)
		{
			TrackBones.AddUnique(Bone);
		}
		else if (Track.Kind == EAnimStitchTrackKind::IKLimb)
		{
			const FAnimStitchFootChain Chain = FAnimStitchManualEdits::ChainFor(RefSkel, Track.Bone);
			if (Chain.IsValid())
			{
				TrackBones.AddUnique(Chain.Thigh);
				TrackBones.AddUnique(Chain.Calf);
				TrackBones.AddUnique(Chain.Foot);
			}
		}
	}
	TrackBones.Sort();
	// Pelvis-style corrections re-plant the feet with leg IK, so the legs need tracks too.
	FName PlantFeet[2];
	bool bPlantFeet = false;
	for (const FAnimStitchTrack& Track : Timeline.Tracks) { bPlantFeet |= Track.Kind == EAnimStitchTrackKind::Bone && Track.bKeepFeetPlanted && FAnimStitchManualEdits::IsPelvisBone(Track.Bone); }
	if (bPlantFeet)
	{
		FAnimStitchFootLock::ResolveFootBones(Evaluator.GetSkeleton(), Timeline.FootLock, PlantFeet[0], PlantFeet[1]);
		for (const FName FootBone : PlantFeet)
		{
			const FAnimStitchFootChain Chain = FAnimStitchFootLock::BuildChain(RefSkel, FootBone);
			if (Chain.IsValid()) { TrackBones.AddUnique(Chain.Thigh); TrackBones.AddUnique(Chain.Calf); TrackBones.AddUnique(Chain.Foot); }
			else { bPlantFeet = false; }
		}
		TrackBones.Sort();
	}
	if (bFootLock || bPredict)
	{
		FName Left, Right;
		FAnimStitchFootLock::ResolveFootBones(Evaluator.GetSkeleton(), Timeline.FootLock, Left, Right);
		for (const FName FootBone : { Left, Right })
		{
			const FAnimStitchFootChain Chain = FAnimStitchFootLock::BuildChain(RefSkel, FootBone);
			if (!Chain.IsValid())
			{
				OutError = FText::Format(LOCTEXT("ErrFootLockBone", "Foot lock is on, but the foot bone '{0}' was not found or has no thigh/calf above it. Pick the foot bones in the Bake window."), FText::FromName(FootBone)).ToString();
				return nullptr;
			}
			for (const int32 LegBone : { Chain.Thigh, Chain.Calf, Chain.Foot })
			{
				TrackBones.AddUnique(LegBone);
			}
		}
		TrackBones.Sort();
	}
	constexpr int64 MaxBakeBytes = 1024ll * 1024 * 1024; // a single serialized property must stay well below 2 GB
	if ((int64)NumKeys * TrackBones.Num() * (bKeepPoses ? 100 : 40) > MaxBakeBytes)
	{
		OutError = FText::Format(LOCTEXT("ErrTooLarge", "The bake would need {0} keys on {1} bones, which is too large. Lower the frame rate or shorten the timeline."),
			NumKeys, TrackBones.Num()).ToString();
		return nullptr;
	}

FScopedSlowTask Task(FullFrames + 3, LOCTEXT("BakeProgress", "Baking AnimStitch timeline..."));
	if (Options.bShowProgress)
	{
		Task.MakeDialog(true);
	}

	// Sample the shared evaluator once per output frame.
	TArray<TArray<FVector3f>> Pos;
	TArray<TArray<FQuat4f>> Rot;
	TArray<TArray<FVector3f>> Scl;
	Pos.SetNum(NumBones);
	Rot.SetNum(NumBones);
	Scl.SetNum(NumBones);
	TArray<FTransform> Pose;
	TArray<TArray<FTransform>> FramePoses;
// Without the whole-hierarchy passes only the range itself needs evaluating.
	for (int32 Key = bKeepPoses ? 0 : FirstFrame; Key <= (bKeepPoses ? FullFrames : LastFrame); ++Key)
	{
		if (Options.bShowProgress)
		{
			if (Task.ShouldCancel())
			{
				OutError = LOCTEXT("ErrCancelled", "Bake cancelled.").ToString();
				return nullptr;
			}
			Task.EnterProgressFrame(1.f);
		}
		Evaluator.EvaluatePose(Key / FrameRate.AsDecimal(), Pose);
		if (bKeepPoses)
		{
			FramePoses.Add(Pose); // kept whole: the foot lock pass needs the full hierarchy
			continue;
		}
		for (const int32 b : TrackBones)
		{
			Pos[b].Add(FVector3f(Pose[b].GetTranslation()));
			Rot[b].Add(FQuat4f(Pose[b].GetRotation().GetNormalized()));
			Scl[b].Add(FVector3f(Pose[b].GetScale3D()));
		}
	}

	if (bKeepPoses)
	{
		if (bPredict)
		{
			// EXPERIMENTAL pre-blend prediction: make the crossfades plant-aware before foot lock cleans up residuals.
			const FAnimStitchPredictReport Report = FAnimStitchFootLock::PredictFeet(Evaluator, FramePoses, FrameRate.AsDecimal());
			if (!Report.bValid)
			{
				OutError = Report.Problem.ToString();
				return nullptr;
			}
			UE_LOG(LogTemp, Display, TEXT("AnimStitch foot prediction: %d crossfade(s), %d foot-frames moved, %d released by reach."), Report.Overlaps, Report.PinnedFootFrames, Report.ReachReleasedFrames);
		}
		if (bFootLock)
		{
			// Post-blend correction: detect plants, pin them, solve the legs, then write the corrected transforms.
			const FAnimStitchFootLockResult Detection = FAnimStitchFootLock::Detect(Evaluator, FramePoses, FrameRate.AsDecimal());
			if (!Detection.bValid)
			{
				OutError = Detection.Problem.ToString();
				return nullptr;
			}
			const int32 Corrected = FAnimStitchFootLock::Pin(Evaluator, FramePoses, Detection);
			UE_LOG(LogTemp, Display, TEXT("AnimStitch foot lock: %d foot-frames pinned (ground z %.2f)."), Corrected, Detection.GroundZ);
		}
		if (bManual)
		{
			// Manual edits come last: they win over foot prediction and foot lock. Same function the preview uses.
			for (int32 f = 0; f < FramePoses.Num(); ++f)
			{
				FAnimStitchManualEdits::Apply(RefSkel, Timeline.Tracks, f / FrameRate.AsDecimal(), FramePoses[f], bPlantFeet ? PlantFeet : nullptr);
			}
		}
for (int32 FrameIndex = FirstFrame; FrameIndex <= LastFrame; ++FrameIndex)
{
const TArray<FTransform>& Frame = FramePoses[FrameIndex];
for (const int32 b : TrackBones)
{
				Pos[b].Add(FVector3f(Frame[b].GetTranslation()));
				Rot[b].Add(FQuat4f(Frame[b].GetRotation().GetNormalized()));
				Scl[b].Add(FVector3f(Frame[b].GetScale3D()));
			}
		}
		FramePoses.Empty();
	}

	UPackage* Package = Existing ? Existing->GetOutermost() : CreatePackage(*PackagePath);
	UAnimSequence* Seq = Existing ? Existing : NewObject<UAnimSequence>(Package, *AssetName, RF_Public | RF_Standalone);
	Seq->SetSkeleton(Evaluator.GetSkeleton());
	if (Existing)
	{
		Seq->Notifies.Reset(); // notifies and curves are not carried over in v1
	}

	if (Options.bShowProgress)
	{
		Task.EnterProgressFrame(1.f, LOCTEXT("BakeWriting", "Writing animation data..."));
	}
	IAnimationDataController& Controller = Seq->GetController();
	Controller.OpenBracket(LOCTEXT("BakeBracket", "AnimStitch Bake"), false);
	if (Existing)
	{
		Controller.ResetModel(false);
	}
	Controller.InitializeModel();
	Controller.SetFrameRate(FrameRate, false);
	Controller.SetNumberOfFrames(FFrameNumber(NumFrames), false);
	for (const int32 b : TrackBones)
	{
		// A bone that never moves is written as a single key, like imported static tracks.
		bool bConstant = true;
		for (int32 k = 1; k < NumKeys && bConstant; ++k)
		{
			bConstant = Pos[b][k].Equals(Pos[b][0], 1e-4f)
				&& Scl[b][k].Equals(Scl[b][0], 1e-5f)
				&& FMath::Abs(Rot[b][k] | Rot[b][0]) > 1.f - 1e-8f;
		}
		if (bConstant)
		{
			Pos[b].SetNum(1);
			Rot[b].SetNum(1);
			Scl[b].SetNum(1);
		}
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

	// Provenance: what produced this animation. Only the new sequence is touched, never the sources.
	Seq->RemoveUserDataOfClass(UAnimStitchProvenance::StaticClass());
	UAnimStitchProvenance* Provenance = NewObject<UAnimStitchProvenance>(Seq, NAME_None, RF_Public | RF_Transactional);
	Provenance->Fill(Timeline, Options.SourceTimeline);
	Seq->AddAssetUserData(Provenance);

	Seq->MarkPackageDirty();
	if (!Existing)
	{
		FAssetRegistryModule::AssetCreated(Seq);
	}

	if (Options.bSavePackage)
	{
		if (Options.bShowProgress)
		{
			Task.EnterProgressFrame(1.f, LOCTEXT("BakeSaving", "Saving..."));
		}
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString File = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Package, Seq, *File, Args))
		{
			OutError = LOCTEXT("ErrSave", "Failed to save the baked sequence.").ToString();
			return nullptr;
		}
	}
	if (!IsRunningCommandlet())
	{
		UAnimStitchSettings* Settings = GetMutableDefault<UAnimStitchSettings>();
		Settings->BakedAssets.AddUnique(FSoftObjectPath(Seq));
		Settings->SaveConfig();
		OnAssetBaked().Broadcast();
	}
	return Seq;
}

#undef LOCTEXT_NAMESPACE



