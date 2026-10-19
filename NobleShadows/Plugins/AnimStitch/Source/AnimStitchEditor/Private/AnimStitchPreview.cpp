#include "AnimStitchPreview.h"
#include "AnimStitchPoseComponent.h"
#include "AnimStitchManualEdits.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneManagement.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ReferenceSkeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace
{
	constexpr int32 MaxGhostJoins = 12;
	constexpr double TrajectorySampleRate = 30.0;
}

FAnimStitchPreview::FAnimStitchPreview()
{
	Scene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
	Scene->SetFloorVisibility(true, true);

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent")))
	{
		auto MakeGhost = [Base](const FLinearColor& Color)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetScalarParameterValue(TEXT("Opacity"), 0.35f);
			return MID;
		};
		OutgoingGhostMaterial = MakeGhost(FLinearColor(1.f, 0.55f, 0.1f));
		IncomingGhostMaterial = MakeGhost(FLinearColor(0.15f, 0.8f, 1.f));
	}
}

FAnimStitchPreview::~FAnimStitchPreview()
{
}

void FAnimStitchPreview::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Mesh);
	Collector.AddReferencedObject(MainComponent);
	Collector.AddReferencedObjects(GhostComponents);
	Collector.AddReferencedObjects(AttachmentComponents);
	Collector.AddReferencedObject(OutgoingGhostMaterial);
	Collector.AddReferencedObject(IncomingGhostMaterial);
}

bool FAnimStitchPreview::IsReady() const
{
	return bEvaluatorValid && Mesh && Mesh->GetSkeleton() == Evaluator.GetSkeleton();
}

void FAnimStitchPreview::SetMesh(USkeletalMesh* InMesh)
{
	Mesh = InMesh;
	if (SelectedBone != INDEX_NONE)
	{
		SelectedBone = INDEX_NONE; // bone indices belong to the old mesh
		OnBoneSelectionChanged.Broadcast();
	}
	RebuildComponents();
	RebuildDerivedData();
	SetPlayhead(Playhead);
}

void FAnimStitchPreview::SetCorrectedPoses(TArray<TArray<FTransform>>&& Poses, double Fps)
{
	CorrectedPoses = MoveTemp(Poses);
	CorrectedFps = Fps;
	SetPlayhead(Playhead);
}

void FAnimStitchPreview::ClearCorrectedPoses()
{
	if (CorrectedPoses.Num() > 0)
	{
		CorrectedPoses.Reset();
		SetPlayhead(Playhead);
	}
}

void FAnimStitchPreview::SetTimeline(const FAnimStitchTimeline& Timeline)
{
	bPlantFeetResolved = false;
	CorrectedPoses.Reset(); // stale until the foot lock pass has been re-run for the new timeline
	ManualTracks = Timeline.Tracks;
	FString Error;
	bEvaluatorValid = Timeline.Clips.Num() > 0 && Evaluator.Initialize(Timeline, Error);
	if (Timeline.Clips.Num() == 0)
	{
		Problem = LOCTEXT("ProblemNoClips", "Add clips to the timeline to preview them.");
	}
	else if (!bEvaluatorValid)
	{
		Problem = FText::FromString(Error);
	}
	RebuildDerivedData();
	SetPlayhead(Playhead);
}

void FAnimStitchPreview::RebuildComponents()
{
	if (MainComponent)
	{
		Scene->RemoveComponent(MainComponent);
		MainComponent = nullptr;
	}
	for (UAnimStitchPoseComponent* Ghost : GhostComponents)
	{
		Scene->RemoveComponent(Ghost);
	}
	GhostComponents.Reset();
	for (UPrimitiveComponent* Prop : AttachmentComponents)
	{
		if (Prop) { Scene->RemoveComponent(Prop); } // they hang on the old character component
	}
	AttachmentComponents.Reset();

	if (!Mesh)
	{
		return;
	}
	MainComponent = NewObject<UAnimStitchPoseComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	MainComponent->SetSkinnedAssetAndUpdate(Mesh);
	Scene->AddComponent(MainComponent, FTransform::Identity);
	RebuildAttachments(); // props are attached to this component
}

void FAnimStitchPreview::RebuildDerivedData()
{
	TrajectoryPoints.Reset();
	GhostPoses.Reset();

	if (bEvaluatorValid && Mesh && Mesh->GetSkeleton() != Evaluator.GetSkeleton())
	{
		Problem = FText::Format(LOCTEXT("ProblemSkeleton", "Preview mesh uses skeleton '{0}' but the timeline uses '{1}'. Pick a matching mesh."),
			FText::FromString(GetNameSafe(Mesh->GetSkeleton())), FText::FromString(GetNameSafe(Evaluator.GetSkeleton())));
	}
	else if (bEvaluatorValid && !Mesh)
	{
		Problem = LOCTEXT("ProblemNoMesh", "Choose a preview skeletal mesh.");
	}
	else if (bEvaluatorValid)
	{
		Problem = FText::GetEmpty();
	}

	// Ghost components are rebuilt to match the current number of joins.
	for (UAnimStitchPoseComponent* Ghost : GhostComponents)
	{
		Scene->RemoveComponent(Ghost);
	}
	GhostComponents.Reset();

	if (!IsReady())
	{
		return;
	}

	const FAnimStitchTimeline& TL = Evaluator.GetTimeline();
	const double Length = TL.GetTotalLength();
	for (double T = 0.0; T < Length + 0.5 / TrajectorySampleRate; T += 1.0 / TrajectorySampleRate)
	{
		TrajectoryPoints.Add(Evaluator.EvaluateRoot(FMath::Min(T, Length)).GetTranslation());
	}

	const int32 NumJoins = FMath::Min(TL.Clips.Num() - 1, MaxGhostJoins);
	for (int32 Join = 1; Join <= NumJoins; ++Join)
	{
		TArray<FTransform>& Out = GhostPoses.AddDefaulted_GetRef();
		Evaluator.EvaluateClipPose(Join - 1, TL.Clips[Join - 1].GetEndTime(), Out);
		TArray<FTransform>& In = GhostPoses.AddDefaulted_GetRef();
		Evaluator.EvaluateClipPose(Join, TL.Clips[Join].StartTime, In);

		for (int32 Side = 0; Side < 2; ++Side)
		{
			UAnimStitchPoseComponent* Ghost = NewObject<UAnimStitchPoseComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			Ghost->SetSkinnedAssetAndUpdate(Mesh);
			UMaterialInterface* Material = Side == 0 ? Cast<UMaterialInterface>(OutgoingGhostMaterial) : Cast<UMaterialInterface>(IncomingGhostMaterial);
			if (Material)
			{
				for (int32 Slot = 0; Slot < Ghost->GetNumMaterials(); ++Slot)
				{
					Ghost->SetMaterial(Slot, Material);
				}
			}
			Ghost->SetCastShadow(false);
			Ghost->SetVisibility(bShowGhosts);
			Scene->AddComponent(Ghost, FTransform::Identity);
			GhostComponents.Add(Ghost);
		}
	}
	ApplyGhostPoses();
}

void FAnimStitchPreview::ApplyGhostPoses()
{
	for (int32 i = 0; i < GhostComponents.Num() && i < GhostPoses.Num(); ++i)
	{
		GhostComponents[i]->ApplySkeletonPose(Evaluator.GetSkeleton(), GhostPoses[i]);
	}
}

void FAnimStitchPreview::SetPlayhead(double Time)
{
	struct FRedrawOnExit { const FAnimStitchPreview* Self; ~FRedrawOnExit() { Self->RequestRedraw(); } } RedrawOnExit{ this };
	Playhead = Time;
	if (!bEvaluatorValid && MainComponent && Mesh && Mesh->GetSkeleton())
	{
		// Nothing to play (new / empty timeline): show the skeleton's default pose instead of a stale frame.
		MainComponent->ApplySkeletonPose(Mesh->GetSkeleton(), Mesh->GetSkeleton()->GetReferenceSkeleton().GetRefBonePose());
		return;
	}
	if (IsReady() && MainComponent)
	{
		TArray<FTransform> Pose;
		if (CorrectedPoses.Num() > 1 && (Evaluator.GetTimeline().FootLock.bEnabled || Evaluator.GetTimeline().bPredictFeet))
		{
			// Foot lock is on: play the corrected frames, interpolated between the two nearest.
			const double Position = FMath::Clamp(Time * CorrectedFps, 0.0, double(CorrectedPoses.Num() - 1));
			const int32 F0 = FMath::FloorToInt32(Position);
			const int32 F1 = FMath::Min(F0 + 1, CorrectedPoses.Num() - 1);
			const double Alpha = Position - F0;
			Pose = CorrectedPoses[F0];
			if (Alpha > 1e-6 && F1 != F0)
			{
				for (int32 b = 0; b < Pose.Num(); ++b)
				{
					const FTransform& A = CorrectedPoses[F0][b];
					const FTransform& B = CorrectedPoses[F1][b];
					Pose[b] = FTransform(FQuat::Slerp(A.GetRotation(), B.GetRotation(), Alpha).GetNormalized(),
						FMath::Lerp(A.GetTranslation(), B.GetTranslation(), Alpha), FMath::Lerp(A.GetScale3D(), B.GetScale3D(), Alpha));
				}
			}
		}
		else
		{
			Evaluator.EvaluatePose(Time, Pose);
		}
		// Manual corrections come last: they win over foot prediction and foot lock.
		// Keep the pose before the manual corrections: IK controls read it, so editing the pelvis does not drag them along.
		bHaveBasePose = false;
		for (const FAnimStitchTrack& T : ManualTracks)
		{
			if (T.Kind == EAnimStitchTrackKind::IKLimb)
			{
				const FReferenceSkeleton& SkelRef = Evaluator.GetSkeleton()->GetReferenceSkeleton();
				BaseLocalPose = Pose;
				BaseComponentPose.SetNumUninitialized(Pose.Num());
				for (int32 i = 0; i < Pose.Num(); ++i)
				{
					const int32 Parent = SkelRef.GetParentIndex(i);
					BaseComponentPose[i] = (Parent != INDEX_NONE && Parent < i) ? Pose[i] * BaseComponentPose[Parent] : Pose[i];
				}
				bHaveBasePose = true;
				break;
			}
		}
		if (!bPlantFeetResolved)
		{
			FAnimStitchFootLock::ResolveFootBones(Evaluator.GetSkeleton(), Evaluator.GetTimeline().FootLock, PlantFeet[0], PlantFeet[1]);
			bPlantFeetResolved = true;
		}
		FAnimStitchManualEdits::Apply(Evaluator.GetSkeleton()->GetReferenceSkeleton(), ManualTracks, Time, Pose, PlantFeet);
		if (bPinRoot && Pose.Num() > 0)
		{
			FVector Loc = Pose[0].GetTranslation();
			Loc.X = 0.0;
			Loc.Y = 0.0;
			Pose[0].SetTranslation(Loc);
		}
		MainComponent->ApplySkeletonPose(Evaluator.GetSkeleton(), Pose);
	}
}

int32 FAnimStitchPreview::GetNumMeshBones() const
{
	return Mesh ? Mesh->GetRefSkeleton().GetNum() : 0;
}

FName FAnimStitchPreview::GetMeshBoneName(int32 MeshBone) const
{
	return Mesh && Mesh->GetRefSkeleton().IsValidIndex(MeshBone) ? Mesh->GetRefSkeleton().GetBoneName(MeshBone) : NAME_None;
}

int32 FAnimStitchPreview::GetMeshBoneParent(int32 MeshBone) const
{
	return Mesh && Mesh->GetRefSkeleton().IsValidIndex(MeshBone) ? Mesh->GetRefSkeleton().GetParentIndex(MeshBone) : INDEX_NONE;
}

int32 FAnimStitchPreview::FindMeshBone(FName BoneName) const
{
	return Mesh ? Mesh->GetRefSkeleton().FindBoneIndex(BoneName) : INDEX_NONE;
}

void FAnimStitchPreview::SelectBone(int32 MeshBone)
{
	if (MeshBone != INDEX_NONE && !(Mesh && Mesh->GetRefSkeleton().IsValidIndex(MeshBone)))
	{
		MeshBone = INDEX_NONE;
	}
	if (SelectedBone != MeshBone)
	{
		SelectedBone = MeshBone;
		OnBoneSelectionChanged.Broadcast();
		RequestRedraw();
	}
}

bool FAnimStitchPreview::GetSelectedBoneTransforms(FTransform& OutLocal, FTransform& OutComponent) const
{
	if (!MainComponent || SelectedBone == INDEX_NONE)
	{
		return false;
	}
	const TArray<FTransform>& Local = MainComponent->GetBoneSpaceTransforms();
	const TArray<FTransform>& Component = MainComponent->GetComponentSpaceTransforms();
	if (!Local.IsValidIndex(SelectedBone) || !Component.IsValidIndex(SelectedBone))
	{
		return false;
	}
	OutLocal = Local[SelectedBone];
	OutComponent = Component[SelectedBone];
	return true;
}

int32 FAnimStitchPreview::PickBone(const FMatrix& ViewProjection, const FIntRect& ViewRect, const FVector2D& Pixel, float MaxDistancePixels) const
{
	if (!MainComponent || !Mesh)
	{
		return INDEX_NONE;
	}
	const TArray<FTransform>& Component = MainComponent->GetComponentSpaceTransforms();
	int32 Best = INDEX_NONE;
	double BestDistance = MaxDistancePixels;
	for (int32 i = 0; i < Component.Num(); ++i)
	{
		FVector2D Screen;
		if (FSceneView::ProjectWorldToScreen(MainComponent->GetComponentTransform().TransformPosition(Component[i].GetTranslation()), ViewRect, ViewProjection, Screen))
		{
			const double Distance = FVector2D::Distance(Screen, Pixel);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = i;
			}
		}
	}
	return Best;
}

void FAnimStitchPreview::GetDrawnBones(TArray<bool>& OutDrawn) const
{
	const int32 Num = GetNumMeshBones();
	OutDrawn.Init(false, Num);
	if (!Mesh || BoneDrawMode == EAnimStitchBoneDrawMode::None) { return; }
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	if (BoneDrawMode == EAnimStitchBoneDrawMode::AllHierarchy)
	{
		OutDrawn.Init(true, Num);
		return;
	}
	if (!Ref.IsValidIndex(SelectedBone)) { return; }
	OutDrawn[SelectedBone] = true;
	const bool bParents = BoneDrawMode == EAnimStitchBoneDrawMode::SelectedAndParents || BoneDrawMode == EAnimStitchBoneDrawMode::SelectedAndParentsAndChildren;
	const bool bChildren = BoneDrawMode == EAnimStitchBoneDrawMode::SelectedAndChildren || BoneDrawMode == EAnimStitchBoneDrawMode::SelectedAndParentsAndChildren;
	if (bParents)
	{
		for (int32 P = Ref.GetParentIndex(SelectedBone); P != INDEX_NONE; P = Ref.GetParentIndex(P)) { OutDrawn[P] = true; }
	}
	if (bChildren)
	{
		// Bones are ordered parent-before-child, so one forward pass finds every descendant of the selected bone
		// (tracked separately so that siblings reached through the parent chain are not mistaken for descendants).
		TArray<bool> Descendant;
		Descendant.Init(false, Num);
		Descendant[SelectedBone] = true;
		for (int32 i = SelectedBone + 1; i < Num; ++i)
		{
			const int32 P = Ref.GetParentIndex(i);
			if (P != INDEX_NONE && Descendant[P])
			{
				Descendant[i] = true;
				OutDrawn[i] = true;
			}
		}
	}
}
void FAnimStitchPreview::DrawBoneNames(const FSceneView& View, FCanvas& Canvas, float DPIScale) const
{
	if (!bShowBoneNames || BoneDrawMode == EAnimStitchBoneDrawMode::None || !MainComponent || !Mesh || !GEngine) { return; }
	const TArray<FTransform>& Component = MainComponent->GetComponentSpaceTransforms();
	TArray<bool> Drawn;
	GetDrawnBones(Drawn);
	const FMatrix WorldToClip = View.ViewMatrices.GetWorldToClip();
	for (int32 i = 0; i < Component.Num() && i < Drawn.Num(); ++i)
	{
		FVector2D Pixel;
		if (!Drawn[i] || !FSceneView::ProjectWorldToScreen(MainComponent->GetComponentTransform().TransformPosition(Component[i].GetTranslation()), View.UnconstrainedViewRect, WorldToClip, Pixel))
		{
			continue;
		}
		FCanvasTextItem Text(FVector2D(Pixel.X + 6.0, Pixel.Y - 6.0) / FMath::Max(DPIScale, 0.1f), FText::FromName(Mesh->GetRefSkeleton().GetBoneName(i)), GEngine->GetSmallFont(),
			i == SelectedBone ? FLinearColor(1.f, 0.9f, 0.3f) : FLinearColor::White);
		Text.EnableShadow(FLinearColor::Black);
		Canvas.DrawItem(Text);
	}
}

void FAnimStitchViewportClient::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
	if (const TSharedPtr<FAnimStitchPreview> Pinned = Preview.Pin())
	{
		Pinned->DrawControlsCanvas(View, Canvas, UpdateViewportClientWindowDPIScale());
		Pinned->DrawBoneNames(View, Canvas, UpdateViewportClientWindowDPIScale());
	}
}

void FAnimStitchPreview::SetPreviewAttachments(const TArray<FAnimStitchPreviewAttachment>& NewAttachments)
{
	auto Same = [](const FAnimStitchPreviewAttachment& A, const FAnimStitchPreviewAttachment& B)
	{
		return A.Asset == B.Asset && A.Socket == B.Socket && A.Location.Equals(B.Location) && A.Rotation.Equals(B.Rotation) && A.Scale.Equals(B.Scale);
	};
	bool bChanged = NewAttachments.Num() != Attachments.Num();
	for (int32 i = 0; !bChanged && i < NewAttachments.Num(); ++i) { bChanged = !Same(NewAttachments[i], Attachments[i]); }
	if (bChanged)
	{
		Attachments = NewAttachments;
		RebuildAttachments();
	}
}

void FAnimStitchPreview::RebuildAttachments()
{
	for (UPrimitiveComponent* Component : AttachmentComponents)
	{
		if (Component) { Scene->RemoveComponent(Component); }
	}
	AttachmentComponents.Reset();
	if (!MainComponent) { RequestRedraw(); return; }

	for (const FAnimStitchPreviewAttachment& Attachment : Attachments)
	{
		UObject* Loaded = Attachment.Asset.LoadSynchronous();
		UPrimitiveComponent* Component = nullptr;
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Loaded))
		{
			UStaticMeshComponent* StaticComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			StaticComponent->SetStaticMesh(StaticMesh);
			Component = StaticComponent;
		}
		else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Loaded))
		{
			USkeletalMeshComponent* SkeletalComponent = NewObject<USkeletalMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			SkeletalComponent->SetSkeletalMesh(SkeletalMesh);
			Component = SkeletalComponent;
		}
		if (!Component) { continue; }
		Scene->AddComponent(Component, FTransform::Identity);
		Component->AttachToComponent(MainComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Attachment.Socket);
		Component->SetRelativeTransform(FTransform(Attachment.Rotation, Attachment.Location, Attachment.Scale));
		AttachmentComponents.Add(Component);
	}
	RequestRedraw();
}

void FAnimStitchPreview::SetShowGhosts(bool bShow)
{
	bShowGhosts = bShow;
	for (UAnimStitchPoseComponent* Ghost : GhostComponents)
	{
		Ghost->SetVisibility(bShow);
	}
}

void FAnimStitchPreview::SetShowRootPath(bool bShow)
{
	bShowRootPath = bShow;
}

void FAnimStitchPreview::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	if (View)
	{
		DrawControls(*View, PDI);
	}
	if (bShowGrid)
	{
		// Ground grid: 1 m cells, a stronger line every 5 m, coloured world axes. Lifted slightly to avoid z-fighting with the floor.
		constexpr int32 HalfCells = 20;
		constexpr double Cell = 100.0;
		constexpr double Z = 0.5;
		const double Extent = HalfCells * Cell;
		// Fine 10 cm lines near the origin (+-5 m).
		constexpr int32 FineHalf = 50;
		constexpr double FineCell = 10.0;
		for (int32 i = -FineHalf; i <= FineHalf; ++i)
		{
			if ((i % 10) == 0) { continue; } // the 1 m lines are drawn below
			const FLinearColor Fine(0.2f, 0.2f, 0.2f); // minor subdivisions: dark gray
			PDI->DrawLine(FVector(-FineHalf * FineCell, i * FineCell, Z), FVector(FineHalf * FineCell, i * FineCell, Z), Fine, SDPG_World, 0.f);
			PDI->DrawLine(FVector(i * FineCell, -FineHalf * FineCell, Z), FVector(i * FineCell, FineHalf * FineCell, Z), Fine, SDPG_World, 0.f);
		}
		for (int32 i = -HalfCells; i <= HalfCells; ++i)
		{
			const bool bAxis = i == 0;
			const bool bMajor = (i % 5) == 0;
			const FLinearColor Major(0.f, 0.f, 0.f); // major lines: black
			const FLinearColor LineX = bAxis ? FLinearColor(0.9f, 0.2f, 0.2f) : Major;
			const FLinearColor LineY = bAxis ? FLinearColor(0.2f, 0.9f, 0.2f) : Major;
			PDI->DrawLine(FVector(-Extent, i * Cell, Z), FVector(Extent, i * Cell, Z), LineX, SDPG_World, bMajor ? 1.f : 0.f);
			PDI->DrawLine(FVector(i * Cell, -Extent, Z), FVector(i * Cell, Extent, Z), LineY, SDPG_World, bMajor ? 1.f : 0.f);
		}
	}
	if (BoneDrawMode != EAnimStitchBoneDrawMode::None && MainComponent && Mesh)
	{
		const TArray<FTransform>& Component = MainComponent->GetComponentSpaceTransforms();
		const FReferenceSkeleton& RefSkel = Mesh->GetRefSkeleton();
		if (Component.Num() == RefSkel.GetNum())
		{
			TArray<bool> Drawn;
			GetDrawnBones(Drawn);
			const FTransform& ToWorld = MainComponent->GetComponentTransform();
			auto World = [&](int32 Bone) { return ToWorld.TransformPosition(Component[Bone].GetTranslation()); };
			// Depth in the hierarchy, used to colour chains differently ("multi-color bones").
			auto BoneColor = [&](int32 Bone, float Brightness)
			{
				if (!bMultiColorBones) { return FLinearColor(0.3f, 0.75f, 1.f) * Brightness; }
				int32 Depth = 0;
				for (int32 P = RefSkel.GetParentIndex(Bone); P != INDEX_NONE; P = RefSkel.GetParentIndex(P)) { ++Depth; }
				return FLinearColor::MakeFromHSV8(uint8((Depth * 37) & 0xFF), 200, uint8(255.f * Brightness));
			};
			const float Size = BoneDrawSize;
			// Animation Sequence editor style: every bone is an elongated octahedron from its parent joint to its own joint.
			auto DrawBoneShape = [&](const FVector& Start, const FVector& End, const FLinearColor& Color, float Thickness)
			{
				const FVector Axis = End - Start;
				const double Length = Axis.Size();
				if (Length < 0.01) { return; }
				const FVector Dir = Axis / Length;
				const FVector Side0 = FVector::CrossProduct(Dir, FMath::Abs(Dir.Z) < 0.9 ? FVector::UpVector : FVector::RightVector).GetSafeNormal();
				const FVector Side1 = FVector::CrossProduct(Dir, Side0);
				const double Width = FMath::Clamp(Length * 0.12, 0.35, 3.0) * Size;
				const FVector Mid = Start + Dir * Length * 0.15;
				const FVector Ring[4] = { Mid + Side0 * Width, Mid + Side1 * Width, Mid - Side0 * Width, Mid - Side1 * Width };
				for (int32 k = 0; k < 4; ++k)
				{
					PDI->DrawLine(Start, Ring[k], Color, SDPG_Foreground, Thickness);
					PDI->DrawLine(End, Ring[k], Color, SDPG_Foreground, Thickness);
					PDI->DrawLine(Ring[k], Ring[(k + 1) % 4], Color, SDPG_Foreground, Thickness);
				}
			};
			for (int32 i = 0; i < Component.Num(); ++i)
			{
				if (!Drawn[i]) { continue; }
				const int32 Parent = RefSkel.GetParentIndex(i);
				if (Parent != INDEX_NONE)
				{
					DrawBoneShape(World(Parent), World(i), BoneColor(i, 1.f), 0.f);
				}
				else
				{
					PDI->DrawPoint(World(i), BoneColor(i, 1.f), 5.f * Size, SDPG_Foreground);
				}
			}			if (Component.IsValidIndex(SelectedBone) && Drawn[SelectedBone])
			{
				const FVector P = World(SelectedBone);
				const int32 Parent = RefSkel.GetParentIndex(SelectedBone);
				if (Parent != INDEX_NONE)
				{
					DrawBoneShape(World(Parent), P, FLinearColor(1.f, 0.55f, 0.1f), 1.5f);
				}
				PDI->DrawPoint(P, FLinearColor(1.f, 0.9f, 0.1f), 8.f * Size, SDPG_Foreground);
				const FQuat Rotation = ToWorld.GetRotation() * Component[SelectedBone].GetRotation();
				const double AxisLength = 12.0 * Size;
				PDI->DrawLine(P, P + Rotation.GetAxisX() * AxisLength, FLinearColor::Red, SDPG_Foreground, 1.5f);
				PDI->DrawLine(P, P + Rotation.GetAxisY() * AxisLength, FLinearColor::Green, SDPG_Foreground, 1.5f);
				PDI->DrawLine(P, P + Rotation.GetAxisZ() * AxisLength, FLinearColor(0.2f, 0.4f, 1.f), SDPG_Foreground, 1.5f);
			}
			if (bShowSockets)
			{
				for (const USkeletalMeshSocket* Socket : Mesh->GetActiveSocketList())
				{
					const int32 Bone = Socket ? RefSkel.FindBoneIndex(Socket->BoneName) : INDEX_NONE;
					if (Bone == INDEX_NONE) { continue; }
					const FTransform SocketWorld = Socket->GetSocketLocalTransform() * Component[Bone] * ToWorld;
					DrawWireDiamond(PDI, SocketWorld.ToMatrixNoScale(), 4.f * Size, FColor(255, 0, 255), SDPG_Foreground);
				}
			}
		}
	}	if (!bShowRootPath || TrajectoryPoints.Num() < 2)
	{
		return;
	}
	// Animation Sequence editor style: dark gray curve, small division points, dark circles at the start and end.
	const FLinearColor CurveColor(0.12f, 0.12f, 0.12f);
	const FLinearColor DotColor(0.2f, 0.2f, 0.2f);
	auto OnGround = [](const FVector& P) { return FVector(P.X, P.Y, 1.0); };
	for (int32 i = 1; i < TrajectoryPoints.Num(); ++i)
	{
		PDI->DrawLine(OnGround(TrajectoryPoints[i - 1]), OnGround(TrajectoryPoints[i]), CurveColor, SDPG_Foreground, 1.5f);
	}
	// One small division point per second so speed changes and snaps read clearly.
	for (int32 i = 0; i < TrajectoryPoints.Num(); i += int32(TrajectorySampleRate))
	{
		PDI->DrawPoint(OnGround(TrajectoryPoints[i]), DotColor, 4.f, SDPG_Foreground);
	}
	for (const FVector& End : { TrajectoryPoints[0], TrajectoryPoints.Last() })
	{
		const FVector Centre = OnGround(End);
		DrawCircle(PDI, Centre, FVector::ForwardVector, FVector::RightVector, FLinearColor(0.02f, 0.02f, 0.02f), 14.0, 24, SDPG_Foreground, 2.f);
		PDI->DrawPoint(Centre, FLinearColor(0.02f, 0.02f, 0.02f), 9.f, SDPG_Foreground);
	}
}
// ---------------------------------------------------------------------------------------------

FAnimStitchViewportClient::FAnimStitchViewportClient(const TSharedRef<FAnimStitchPreview>& InPreview, const TSharedRef<SEditorViewport>& InViewport)
	: FEditorViewportClient(nullptr, &InPreview->GetScene(), InViewport)
	, Preview(InPreview)
{
	SetRealtime(true);
	SetViewLocation(FVector(0.0, -350.0, 140.0));
	SetViewRotation(FRotator(-8.0, 90.0, 0.0));
	EngineShowFlags.SetSnap(0);
	bSetListenerPosition = false;
	OverrideNearClipPlane(1.f);
}

void FAnimStitchViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
	if (PreviewScene && PreviewScene->GetWorld())
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FAnimStitchViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
	if (const TSharedPtr<FAnimStitchPreview> Pinned = Preview.Pin())
	{
		Pinned->Draw(View, PDI);
	}
}

void FAnimStitchViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
	if (Key != EKeys::LeftMouseButton || Event != IE_Released)
	{
		return;
	}
	if (const TSharedPtr<FAnimStitchPreview> Pinned = Preview.Pin())
	{
		// Control shapes first, then bones.
		const FAnimStitchControlId Control = Pinned->PickControl(View, FVector2D(HitX, HitY));
		if (Control.IsValid())
		{
			Pinned->SelectControl(Control);
			return;
		}
		Pinned->SelectControl(FAnimStitchControlId());
		if (Pinned->GetBoneDrawMode() != EAnimStitchBoneDrawMode::None)
		{
			// Click a joint to select that bone; clicking empty space clears the selection.
			Pinned->SelectBone(Pinned->PickBone(View.ViewMatrices.GetWorldToClip(), View.UnconstrainedViewRect, FVector2D(HitX, HitY), 14.f));
		}
	}
}

void FAnimStitchViewportClient::UpdateDrag(FViewport* InViewport, int32 X, int32 Y)
{
	if (const TSharedPtr<FAnimStitchPreview> Pinned = Preview.Pin())
	{
		if (!InViewport->KeyState(EKeys::LeftMouseButton))
		{
			// The release was missed (button let go outside the viewport): never stay stuck in a drag.
			bGizmoDrag = false;
			Pinned->EndGizmoDrag();
			return;
		}
		FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(InViewport, GetScene(), EngineShowFlags).SetRealtimeUpdate(true));
		if (FSceneView* View = CalcSceneView(&Family))
		{
			Pinned->UpdateGizmoDrag(*View, FVector2D(X, Y));
		}
	}
}

bool FAnimStitchViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	const TSharedPtr<FAnimStitchPreview> Pinned = Preview.Pin();
	if (Pinned && EventArgs.Key == EKeys::LeftMouseButton)
	{
		if (EventArgs.Event == IE_Pressed && bGizmoDrag)
		{
			bGizmoDrag = false;
			Pinned->EndGizmoDrag(); // a stale drag from a missed release
		}
		if (EventArgs.Event == IE_Pressed && !EventArgs.Viewport->KeyState(EKeys::LeftAlt) && !EventArgs.Viewport->KeyState(EKeys::RightAlt))
		{
			FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(EventArgs.Viewport, GetScene(), EngineShowFlags).SetRealtimeUpdate(true));
			if (FSceneView* View = CalcSceneView(&Family))
			{
				if (Pinned->BeginGizmoDrag(*View, FVector2D(EventArgs.Viewport->GetMouseX(), EventArgs.Viewport->GetMouseY())))
				{
					bGizmoDrag = true;
					return true; // the viewport must not orbit while a gizmo handle is dragged
				}
			}
		}
		else if (EventArgs.Event == IE_Released && bGizmoDrag)
		{
			bGizmoDrag = false;
			Pinned->EndGizmoDrag();
			return true;
		}
	}
	if (Pinned && EventArgs.Event == IE_Pressed && !EventArgs.Viewport->KeyState(EKeys::LeftControl) && !EventArgs.Viewport->KeyState(EKeys::LeftAlt) && !EventArgs.Viewport->KeyState(EKeys::LeftShift))
	{
		// W / E / R: translate / rotate / scale gizmo, like the standard editor widget.
		if (EventArgs.Key == EKeys::W) { Pinned->SetGizmoMode(EAnimStitchGizmoMode::Translate); return true; }
		if (EventArgs.Key == EKeys::E) { Pinned->SetGizmoMode(EAnimStitchGizmoMode::Rotate); return true; }
		if (EventArgs.Key == EKeys::R) { Pinned->SetGizmoMode(EAnimStitchGizmoMode::Scale); return true; }
	}
	return FEditorViewportClient::InputKey(EventArgs);
}

void FAnimStitchViewportClient::MouseMove(FViewport* InViewport, int32 X, int32 Y)
{
	if (bGizmoDrag) { UpdateDrag(InViewport, X, Y); return; }
	FEditorViewportClient::MouseMove(InViewport, X, Y);
}

bool FAnimStitchViewportClient::InputAxis(const FInputKeyEventArgs& Args)
{
	if (bGizmoDrag && Args.Viewport && (Args.Key == EKeys::MouseX || Args.Key == EKeys::MouseY))
	{
		UpdateDrag(Args.Viewport, Args.Viewport->GetMouseX(), Args.Viewport->GetMouseY());
		return true;
	}
	return FEditorViewportClient::InputAxis(Args);
}

void FAnimStitchViewportClient::CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (bGizmoDrag) { UpdateDrag(InViewport, InMouseX, InMouseY); return; }
	FEditorViewportClient::CapturedMouseMove(InViewport, InMouseX, InMouseY);
}

void SAnimStitchViewport::HandleRedraw()
{
	if (Client.IsValid())
	{
		Client->Invalidate(); // make the viewport repaint now, even when Slate is throttling (menus open, mouse captured)
	}
}

void SAnimStitchViewport::Construct(const FArguments& InArgs)
{
	Preview = InArgs._Preview;
	SEditorViewport::Construct(SEditorViewport::FArguments());
	Preview->OnRedrawRequested.AddSP(this, &SAnimStitchViewport::HandleRedraw);
}

TSharedRef<FEditorViewportClient> SAnimStitchViewport::MakeEditorViewportClient()
{
	Client = MakeShared<FAnimStitchViewportClient>(Preview.ToSharedRef(), SharedThis(this));
	return Client.ToSharedRef();
}

void SAnimStitchViewport::FocusOnMesh()
{
	if (Client.IsValid() && Preview->GetMesh())
	{
		const FBoxSphereBounds Bounds = Preview->GetMesh()->GetBounds();
		Client->FocusViewportOnBox(Bounds.GetBox(), true);
	}
}

#undef LOCTEXT_NAMESPACE









