#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchFootLock.h"

/** A manual-correction control shape in the viewport. */
enum class EAnimStitchControlKind : uint8 { None, Bone, IKEffector, IKPole, LockPin };

struct FAnimStitchControlId
{
	EAnimStitchControlKind Kind = EAnimStitchControlKind::None;
	/** Bone / IK controls: track index. Lock pin: foot (0 = left, 1 = right). */
	int32 A = INDEX_NONE;
	/** Lock pin: frame inside the lock. */
	int32 B = INDEX_NONE;

	bool IsValid() const { return Kind != EAnimStitchControlKind::None; }
	bool operator==(const FAnimStitchControlId& Other) const { return Kind == Other.Kind && A == Other.A && B == Other.B; }
};

struct FAnimStitchControl
{
	FAnimStitchControlId Id;
	FVector Position = FVector::ZeroVector;
	FQuat Orientation = FQuat::Identity;
	bool bTranslate = true;
	bool bRotate = true;
	bool bScale = true;
	FLinearColor Color = FLinearColor::White;
	FString Label;
};

enum class EAnimStitchGizmoMode : uint8 { Translate, Rotate, Scale };
enum class EAnimStitchCoordSpace : uint8 { World, Local };

class UAnimStitchPoseComponent;

/** Bone drawing modes, same set as the animation editor's Bones menu. */
enum class EAnimStitchBoneDrawMode : uint8
{
	None,
	SelectedOnly,
	SelectedAndParents,
	SelectedAndChildren,
	SelectedAndParentsAndChildren,
	AllHierarchy,
};
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
	void SetShowGrid(bool bShow) { bShowGrid = bShow; RequestRedraw(); }
	bool GetShowGrid() const { return bShowGrid; }

	/**
	 * Foot-lock-corrected poses sampled at Fps for the whole timeline (same Detect + Pin pass the bake uses).
	 * While set, the viewport plays these instead of the raw blend, so what you see is what bakes.
	 */
	void SetCorrectedPoses(TArray<TArray<FTransform>>&& Poses, double Fps);
	void ClearCorrectedPoses();

	/** Hover previews keep the character in place by removing the root's horizontal travel. */
	void SetPinRootInPlace(bool bPin) { bPinRoot = bPin; }

	// ---- Bone display / selection (mesh bone indices) ----
	void SetBoneDrawMode(EAnimStitchBoneDrawMode Mode) { BoneDrawMode = Mode; RequestRedraw(); }
	EAnimStitchBoneDrawMode GetBoneDrawMode() const { return BoneDrawMode; }
	void SetShowBoneNames(bool bShow) { bShowBoneNames = bShow; RequestRedraw(); }
	bool GetShowBoneNames() const { return bShowBoneNames; }
	void SetMultiColorBones(bool bMulti) { bMultiColorBones = bMulti; RequestRedraw(); }
	bool GetMultiColorBones() const { return bMultiColorBones; }
	void SetShowSockets(bool bShow) { bShowSockets = bShow; RequestRedraw(); }
	bool GetShowSockets() const { return bShowSockets; }
	/** Scales joint size, link thickness and axis length of the drawn bones (1 = default). */
	void SetBoneDrawSize(float Size) { BoneDrawSize = FMath::Clamp(Size, 0.1f, 10.f); RequestRedraw(); }
	float GetBoneDrawSize() const { return BoneDrawSize; }
	/** Which mesh bones the current draw mode shows (indexed by mesh bone). */
	void GetDrawnBones(TArray<bool>& OutDrawn) const;
	void DrawBoneNames(const FSceneView& View, FCanvas& Canvas, float DPIScale) const;
	int32 GetNumMeshBones() const;
	FName GetMeshBoneName(int32 MeshBone) const;
	int32 GetMeshBoneParent(int32 MeshBone) const;
	int32 FindMeshBone(FName BoneName) const;
	void SelectBone(int32 MeshBone);
	int32 GetSelectedBone() const { return SelectedBone; }
	/** Live transforms of the selected bone in the pose currently shown: relative to its parent, and in mesh (component) space. */
	bool GetSelectedBoneTransforms(FTransform& OutLocal, FTransform& OutComponent) const;
	/** Nearest bone joint to a screen position (pixels) within MaxDistancePixels, or INDEX_NONE. */
	int32 PickBone(const FMatrix& ViewProjection, const FIntRect& ViewRect, const FVector2D& Pixel, float MaxDistancePixels) const;
	FSimpleMulticastDelegate OnBoneSelectionChanged;

	// ---- Manual correction controls (Phase 9) ----
	/** The timeline asset the controls write keys into (undoable). */
	void SetAsset(class UAnimStitchTimelineAsset* InAsset) { Asset = InAsset; }
	/** Tracks currently applied over the preview pose (copied from the asset). */
	void SetManualTracks(const TArray<FAnimStitchTrack>& Tracks);
	/** Foot lock detection + pin targets, so locked plants can expose their pin as a draggable control. */
	void SetLockInfo(FAnimStitchFootLockResult&& Lock, FAnimStitchPinOutput&& Output, double Fps);
	void ClearLockInfo();

	void BuildControls(TArray<FAnimStitchControl>& OutControls) const;
	FAnimStitchControlId PickControl(const FMatrix& ViewProjection, const FIntRect& ViewRect, const FVector2D& Pixel, float MaxDistancePixels) const;
	void SelectControl(const FAnimStitchControlId& Id);
	FAnimStitchControlId GetSelectedControl() const { return SelectedControl; }
	void SetGizmoMode(EAnimStitchGizmoMode Mode) { GizmoMode = Mode; RequestRedraw(); }
	/** World / local axes for the manipulator. */
	void SetCoordSpace(EAnimStitchCoordSpace Space) { CoordSpace = Space; RequestRedraw(); }
	EAnimStitchCoordSpace GetCoordSpace() const { return CoordSpace; }
	/** Size of the control shapes (the gizmo keeps its own screen size). */
	void SetControlDrawSize(float Size) { ControlDrawSize = FMath::Clamp(Size, 0.2f, 8.f); RequestRedraw(); }
	float GetControlDrawSize() const { return ControlDrawSize; }
	void SetShowControls(bool bShow) { bShowControls = bShow; RequestRedraw(); }
	bool GetShowControls() const { return bShowControls; }

	/** Props shown on the preview character (from the timeline asset). */
	void SetPreviewAttachments(const TArray<FAnimStitchPreviewAttachment>& Attachments);

	/** Asks the viewport to repaint (it only repaints on its own while the UI is not throttled, e.g. not while a menu is open). */
	FSimpleMulticastDelegate OnRedrawRequested;
	void RequestRedraw() const { OnRedrawRequested.Broadcast(); }
	EAnimStitchGizmoMode GetGizmoMode() const { return GizmoMode; }
	void DrawControls(const FSceneView& View, FPrimitiveDrawInterface* PDI) const;

	/** Gizmo dragging: Begin returns true if the press hit a gizmo handle (the viewport must then not orbit). */
	bool BeginGizmoDrag(const FSceneView& View, const FVector2D& Pixel);
	void UpdateGizmoDrag(const FSceneView& View, const FVector2D& Pixel);
	void EndGizmoDrag();
	bool IsGizmoDragging() const { return Drag.bActive; }
	/** Applies a world-space move / rotation / scale of the selected control as a keyframe at the playhead (what dragging does). */
	void ApplyControlEdit(const FVector& TotalTranslation, const FQuat& TotalRotation, const FVector& TotalScale);

	FSimpleMulticastDelegate OnControlSelectionChanged;
	/** The evaluator for the current timeline, or null when the timeline is empty or invalid. */
	const FAnimStitchEvaluator* GetEvaluator() const { return bEvaluatorValid ? &Evaluator : nullptr; }

	/** Empty when everything is fine; otherwise a user-facing reason nothing is shown. */
	FText GetProblem() const { return Problem; }

	void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) const;

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
	EAnimStitchCoordSpace CoordSpace = EAnimStitchCoordSpace::World;
	float ControlDrawSize = 1.f;
	bool bShowControls = true;
	TArray<FAnimStitchPreviewAttachment> Attachments;
	TArray<TObjectPtr<class UPrimitiveComponent>> AttachmentComponents;
	void RebuildAttachments();
	EAnimStitchBoneDrawMode BoneDrawMode = EAnimStitchBoneDrawMode::None;
	bool bShowBoneNames = false;
	bool bMultiColorBones = false;
	bool bShowSockets = false;
	float BoneDrawSize = 1.f;
	int32 SelectedBone = INDEX_NONE;
	bool bPinRoot = false;

	TArray<TArray<FTransform>> CorrectedPoses;
	double CorrectedFps = 30.0;

	// Manual correction state
	TWeakObjectPtr<class UAnimStitchTimelineAsset> Asset;
	TArray<FAnimStitchTrack> ManualTracks;
	FAnimStitchControlId SelectedControl;
	EAnimStitchGizmoMode GizmoMode = EAnimStitchGizmoMode::Translate;
	FAnimStitchFootLockResult LockResult;
	FAnimStitchPinOutput LockOutput;
	double LockFps = 30.0;
	bool bHaveLock = false;

	struct FDragState
	{
		bool bActive = false;
		int32 Axis = 0;                    // 0..2 = X/Y/Z, 3 = screen plane / uniform
		FAnimStitchControl Control;
		double Time = 0.0;
		FAnimStitchTrackKey StartKey;
		FVector StartOffset = FVector::ZeroVector; // lock pin: the offset when the drag began
		FVector StartRayHit = FVector::ZeroVector;
		double StartAxisT = 0.0;
		FVector StartVector = FVector::ZeroVector;
		FVector2D StartPixel = FVector2D::ZeroVector;
		FVector LastTranslation = FVector::ZeroVector;
		FVector Axes[3] = { FVector::ForwardVector, FVector::RightVector, FVector::UpVector };
	} Drag;
	TSharedPtr<class FScopedTransaction> DragTransaction;
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
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual void MouseMove(FViewport* InViewport, int32 X, int32 Y) override;
	virtual void CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;

private:
	void UpdateDrag(FViewport* InViewport, int32 X, int32 Y);

	TWeakPtr<FAnimStitchPreview> Preview;
	bool bGizmoDrag = false;
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
	void HandleRedraw();

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<FAnimStitchViewportClient> Client;
};







