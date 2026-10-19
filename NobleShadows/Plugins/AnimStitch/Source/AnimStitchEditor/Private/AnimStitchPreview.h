#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "AnimStitchTimeline.h"

class UAnimStitchPoseComponent;
class UMaterialInstanceDynamic;
class USkeletalMesh;

/**
 * Owns the preview scene: the posed character, optional join ghosts and the root trajectory.
 * Poses come from FAnimStitchEvaluator, the same code the bake uses.
 */
class FAnimStitchPreview : public FGCObject
{
public:
	FAnimStitchPreview();
	virtual ~FAnimStitchPreview() override;

	FAdvancedPreviewScene& GetScene() { return *Scene; }

	/** Sets the preview character (null clears it). */
	void SetMesh(USkeletalMesh* Mesh);
	USkeletalMesh* GetMesh() const { return Mesh; }

	/** Rebuilds the evaluator, ghosts and trajectory. Call after any timeline edit. */
	void SetTimeline(const FAnimStitchTimeline& Timeline);

	/** Poses the character at the given timeline time. */
	void SetPlayhead(double Time);

	void SetShowGhosts(bool bShow);
	void SetShowRootPath(bool bShow);
	void SetShowGrid(bool bShow) { bShowGrid = bShow; }
	bool GetShowGrid() const { return bShowGrid; }

	/**
	 * Foot-lock-corrected poses sampled at Fps for the whole timeline (same Detect + Pin pass the bake uses).
	 * While set, the viewport plays these instead of the raw blend, so what you see is what bakes.
	 */
	void SetCorrectedPoses(TArray<TArray<FTransform>>&& Poses, double Fps);
	void ClearCorrectedPoses();

	/** Hover previews keep the character in place by removing the root's horizontal travel. */
	void SetPinRootInPlace(bool bPin) { bPinRoot = bPin; }

	/** The evaluator for the current timeline, or null when the timeline is empty or invalid. */
	const FAnimStitchEvaluator* GetEvaluator() const { return bEvaluatorValid ? &Evaluator : nullptr; }

	/** Empty when everything is fine; otherwise a user-facing reason nothing is shown. */
	FText GetProblem() const { return Problem; }

	void Draw(FPrimitiveDrawInterface* PDI) const;

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FAnimStitchPreview"); }

private:
	void RebuildComponents();
	void RebuildDerivedData();
	void ApplyGhostPoses();
	bool IsReady() const;

	TSharedPtr<FAdvancedPreviewScene> Scene;
	TObjectPtr<USkeletalMesh> Mesh;
	TObjectPtr<UAnimStitchPoseComponent> MainComponent;
	TArray<TObjectPtr<UAnimStitchPoseComponent>> GhostComponents;
	TObjectPtr<UMaterialInstanceDynamic> OutgoingGhostMaterial;
	TObjectPtr<UMaterialInstanceDynamic> IncomingGhostMaterial;

	FAnimStitchEvaluator Evaluator;
	bool bEvaluatorValid = false;
	FText Problem;
	double Playhead = 0.0;
	bool bShowGhosts = false;
	bool bShowRootPath = false;
	bool bShowGrid = true;
	bool bPinRoot = false;

	TArray<TArray<FTransform>> CorrectedPoses;
	double CorrectedFps = 30.0;

	TArray<FVector> TrajectoryPoints;
	/** One pose per ghost component (outgoing, incoming, outgoing, ...). */
	TArray<TArray<FTransform>> GhostPoses;
};

class FAnimStitchViewportClient : public FEditorViewportClient
{
public:
	FAnimStitchViewportClient(const TSharedRef<FAnimStitchPreview>& InPreview, const TSharedRef<SEditorViewport>& InViewport);

	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;

private:
	TWeakPtr<FAnimStitchPreview> Preview;
};

class SAnimStitchViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchViewport) {}
		SLATE_ARGUMENT(TSharedPtr<FAnimStitchPreview>, Preview)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Frames the camera on the preview mesh. */
	void FocusOnMesh();

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<FAnimStitchViewportClient> Client;
};

