#include "AnimStitchPreview.h"
#include "AnimStitchPoseComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneManagement.h"

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
	RebuildComponents();
	RebuildDerivedData();
	SetPlayhead(Playhead);
}

void FAnimStitchPreview::SetTimeline(const FAnimStitchTimeline& Timeline)
{
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

	if (!Mesh)
	{
		return;
	}
	MainComponent = NewObject<UAnimStitchPoseComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	MainComponent->SetSkinnedAssetAndUpdate(Mesh);
	Scene->AddComponent(MainComponent, FTransform::Identity);
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
	Playhead = Time;
	if (IsReady() && MainComponent)
	{
		TArray<FTransform> Pose;
		Evaluator.EvaluatePose(Time, Pose);
		MainComponent->ApplySkeletonPose(Evaluator.GetSkeleton(), Pose);
	}
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

void FAnimStitchPreview::Draw(FPrimitiveDrawInterface* PDI) const
{
	if (!bShowRootPath || TrajectoryPoints.Num() < 2)
	{
		return;
	}
	const FLinearColor Color(1.f, 0.9f, 0.1f);
	auto OnGround = [](const FVector& P) { return FVector(P.X, P.Y, 1.0); };
	for (int32 i = 1; i < TrajectoryPoints.Num(); ++i)
	{
		PDI->DrawLine(OnGround(TrajectoryPoints[i - 1]), OnGround(TrajectoryPoints[i]), Color, SDPG_Foreground, 1.5f);
	}
	// One tick per second so speed changes and snaps read clearly.
	for (int32 i = 0; i < TrajectoryPoints.Num(); i += int32(TrajectorySampleRate))
	{
		PDI->DrawPoint(OnGround(TrajectoryPoints[i]), FLinearColor::White, 6.f, SDPG_Foreground);
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
		Pinned->Draw(PDI);
	}
}

void SAnimStitchViewport::Construct(const FArguments& InArgs)
{
	Preview = InArgs._Preview;
	SEditorViewport::Construct(SEditorViewport::FArguments());
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
