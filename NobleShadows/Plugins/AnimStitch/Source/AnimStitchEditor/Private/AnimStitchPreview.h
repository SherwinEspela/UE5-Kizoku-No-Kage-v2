#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchFootLock.h"
#include "Application/ThrottleManager.h"

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
	/** Colour style: 0 centre, 1 left, 2 right. */
	int32 Style = 0;
	/** Half-size of the cube in cm before the global multiplier (from the bone's size). */
	float BaseSize = 4.f;
	/** Per-control look from the track (negative = use the global value). */
	float Opacity = -1.f;
	float SizeMul = -1.f;
	bool bSphere = false;
	/** Display offset (cm): moves the drawn control and its manipulator, not the value it edits. */
	FVector Offset = FVector::ZeroVector;
	/** Pole controls: the IK limb track they belong to (the control's own track is the pole lane). */
	int32 ParentTrack = INDEX_NONE;
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
	void BuildControlsFromPose(TArray<FAnimStitchControl>& OutControls) const;
	/** Picks the control cube under the cursor (the cube's whole screen area counts, not just its centre). */
	FAnimStitchControlId PickControl(const FSceneView& View, const FVector2D& Pixel) const;
	void SelectControl(const FAnimStitchControlId& Id);
	/** The IK limb of a track as it is on screen right now (effector + pole). False if the track is not a valid IK limb. */
	/**
	 * The control of a track as it is evaluated on screen right now: IK limbs (effector + pole) and ABSOLUTE bone tracks (full component-space
	 * transform of the bone). Used to key without popping and by the value panel.
	 */
	bool GetIKLiveKey(int32 TrackIndex, FAnimStitchTrackKey& OutKey) const;
	/** Component-space transform of a bone as currently displayed (including every correction). */
	bool GetBoneComponentTransform(FName Bone, FTransform& OutTransform) const;
	FAnimStitchControlId GetSelectedControl() const { return SelectedControl; }
	void SetGizmoMode(EAnimStitchGizmoMode Mode) { GizmoMode = Mode; RequestRedraw(); }
	/** World / local axes for the manipulator; each tool (move, rotate, scale) remembers its own. Set / Get act on the active tool. */
	void SetCoordSpace(EAnimStitchCoordSpace Space) { CoordSpaces[(int32)GizmoMode] = Space; RequestRedraw(); }
	EAnimStitchCoordSpace GetCoordSpace() const { return CoordSpaces[(int32)GizmoMode]; }
	/** Size of the transform manipulator (1 = default). */
	void SetGizmoDrawSize(float Size) { GizmoDrawSize = FMath::Clamp(Size, 0.3f, 4.f); RequestRedraw(); }
	float GetGizmoDrawSize() const { return GizmoDrawSize; }
	/** Size of the control shapes (the gizmo keeps its own screen size). */
	void SetControlDrawSize(float Size) { ControlDrawSize = FMath::Clamp(Size, 0.2f, 8.f); RequestRedraw(); }
	float GetControlDrawSize() const { return ControlDrawSize; }
	void SetControlOpacity(float Opacity) { ControlOpacity = FMath::Clamp(Opacity, 0.02f, 1.f); RequestRedraw(); }
	float GetControlOpacity() const { return ControlOpacity; }
	/** Translucent cube faces drawn on the viewport canvas (a real blend over the mesh). */
	void DrawControlsCanvas(const FSceneView& View, FCanvas& Canvas, float DPIScale) const;
	/** Auto size of a bone's control cube (half-extent, cm): big for the pelvis, small for fingers. */
	float GetBoneControlSize(int32 MeshBone) const;
	void SetShowControls(bool bShow) { bShowControls = bShow; RequestRedraw(); }
	/** G: hides / shows every helper at once (control shapes, manipulator, grid), like the regular viewport's game view. */
	void ToggleHelpers();
	/** Floor slab (viewport helper): shown with the helpers, colour and side length (m) are editable. */
	void SetShowFloor(bool bShow) { bShowFloor = bShow; UpdateFloor(); }
	bool GetShowFloor() const { return bShowFloor; }
	void SetFloorColor(const FLinearColor& Color) { FloorColor = Color; UpdateFloor(); }
	FLinearColor GetFloorColor() const { return FloorColor; }
	void SetFloorSize(float Meters) { FloorSizeMeters = FMath::Clamp(Meters, 2.f, 1000.f); UpdateFloor(); }
	float GetFloorSize() const { return FloorSizeMeters; }
	bool AreHelpersVisible() const { return bHelpersVisible; }
	/** Test hook: runs the same path as dragging a control by Translation (no viewport needed). */
	bool DebugDragControl(const FAnimStitchControlId& Id, const FVector& Translation);
	/** What F frames: the selected control, else the selected bone, else the whole mesh. */
	bool GetFocusBox(FBox& OutBox) const;
	/** World box around the character as currently posed (all bones, padded); false without a mesh. */
	bool GetCharacterBox(FBox& OutBox) const;
	bool GetShowControls() const { return bShowControls; }

	/** Attached Props tab: the prop highlighted in the viewport (INDEX_NONE = none). */
	void SelectAttachment(int32 Index) { SelectedAttachment = Index; RequestRedraw(); }
	int32 GetSelectedAttachment() const { return SelectedAttachment; }

	/** Props shown on the preview character (from the timeline asset). */
	void SetPreviewAttachments(const TArray<FAnimStitchPreviewAttachment>& Attachments);

	/** Asks the viewport to repaint (it only repaints on its own while the UI is not throttled, e.g. not while a menu is open). */
	FSimpleMulticastDelegate OnRedrawRequested;
	void RequestRedraw() const { OnRedrawRequested.Broadcast(); }
	EAnimStitchGizmoMode GetGizmoMode() const { return GizmoMode; }
	void DrawControls(const FSceneView& View, FPrimitiveDrawInterface* PDI) const;

	// ---- Motion trails (Phase 20) ----
	/** The selected control's evaluated path with a point at every key; follows the G helper toggle. */
	void DrawTrail(const FSceneView& View, FPrimitiveDrawInterface* PDI) const;
	/** Trail key point under the cursor (index into the trail's points), or INDEX_NONE. */
	int32 PickTrailKey(const FSceneView& View, const FVector2D& Pixel) const;
	/** Draggable trail (IK target, pole, root offset) as opposed to a display-only FK bone trail. */
	bool IsTrailEditable() const;
	/** Press on an editable trail point: starts moving that key (one undo transaction until EndGizmoDrag). */
	bool BeginTrailDrag(const FSceneView& View, const FVector2D& Pixel);
	int32 GetNumTrailKeys() const;
	/** Trail points currently shown for one control (0 when its trail is switched off or helpers are hidden with G). */
	int32 GetNumTrailKeysFor(const FAnimStitchControlId& Id) const;
	/** Test hook: the drawn geometry of one control's trail (polyline segments and key points). */
	void GetTrailGeometry(const FAnimStitchControlId& Id, TArray<TArray<FVector>>& OutSegments, TArray<FVector>& OutKeyPoints) const;
	/** Trail point at (nearly) this key time, or INDEX_NONE. */
	int32 FindTrailKey(double Time) const;
	/** Test hook: moves one trail point by Translation through the same path as dragging it (opens and closes the transaction). */
	bool DebugDragTrailKey(int32 TrailKey, const FVector& Translation);

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
	void CreateFloor();
	void UpdateFloor();
	void RebuildDerivedData();
	void ApplyGhostPoses();
	bool IsReady() const;

	TSharedPtr<FAdvancedPreviewScene> Scene;
	TObjectPtr<USkeletalMesh> Mesh;
	TObjectPtr<UAnimStitchPoseComponent> MainComponent;
	TObjectPtr<class UStaticMeshComponent> FloorComponent;
	TObjectPtr<UMaterialInstanceDynamic> FloorMaterial;
	bool bShowFloor = true;
	FLinearColor FloorColor = FLinearColor(0.55f, 0.55f, 0.55f);
	float FloorSizeMeters = 100.f;
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
	EAnimStitchCoordSpace CoordSpaces[3] = { EAnimStitchCoordSpace::World, EAnimStitchCoordSpace::Local, EAnimStitchCoordSpace::World }; // rotate defaults to local
	float GizmoDrawSize = 2.f;
	/** Translucency of the control cubes (0 = invisible, 1 = solid). */
	float ControlOpacity = 0.25f;
	/** Skeleton-indexed pose before the manual corrections (kept for IK controls, so they do not follow body edits). */
	TArray<FTransform> BaseLocalPose;
	TArray<FTransform> BaseComponentPose;
	bool bHaveBasePose = false;
	mutable TArray<float> BoneExtentSize;
	/** Left / right foot bones that bone corrections keep planted (resolved lazily from the foot-lock settings). */
	mutable FName PlantFeet[2];
	mutable bool bPlantFeetResolved = false;
	float ControlDrawSize = 1.f;
	bool bShowControls = true;
	bool bHelpersVisible = true;
	TArray<FAnimStitchPreviewAttachment> Attachments;
	TArray<TObjectPtr<class UPrimitiveComponent>> AttachmentComponents; // aligned with Attachments (null where an asset failed to load)
	int32 SelectedAttachment = INDEX_NONE;
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
		/** Absolute bone tracks: the bone and the root bone in component space when the drag began (keys are root-relative). */
		FTransform StartBoneComp = FTransform::Identity;
		FTransform StartRootComp = FTransform::Identity;
		FVector StartOffset = FVector::ZeroVector; // lock pin: the offset when the drag began
		FVector StartRayHit = FVector::ZeroVector;
		double StartAxisT = 0.0;
		FVector StartVector = FVector::ZeroVector;
		FVector2D StartPixel = FVector2D::ZeroVector;
		FVector LastTranslation = FVector::ZeroVector;
		/** Dragging a motion-trail key point (screen-plane move) instead of a gizmo handle. */
		bool bTrail = false;
		FQuat LastRotation = FQuat::Identity;
		FVector Axes[3] = { FVector::ForwardVector, FVector::RightVector, FVector::UpVector };
	} Drag;
	TSharedPtr<class FScopedTransaction> DragTransaction;
	FThrottleRequest DragThrottle;
	int32 DragUpdates = 0;
	int32 DragWrites = 0;
	TArray<FVector> TrajectoryPoints;
	/** TrajectoryPoints without the Root offset track (so the root-offset trail can follow live edits). */
	TArray<FVector> TrajectoryBase;
	struct FTrailKey { FAnimStitchControlId Control; bool bEditable = false; int32 KeyIndex = INDEX_NONE; double Time = 0.0; FVector Pos = FVector::ZeroVector; };
	struct FTrailSegment { FAnimStitchControlId Control; bool bEditable = false; TArray<FVector> Points; };
	void BuildTrailFor(const FAnimStitchControlId& Id) const;
	void RebuildTrail() const;
	void EnsureTrail() const;
	const TArray<FAnimStitchTrack>& GetTrailTracks() const;
	uint32 ComputeTrailSignature() const;
	mutable uint32 TrailSignature = 0;
	bool IsHeavyTrail() const;
	bool StartTrailDrag(int32 TrailKey, const FVector& StartHit, const FVector2D& Pixel);
	mutable bool bTrailDirty = true;
	mutable TArray<FTrailSegment> TrailSegments;
	mutable TArray<FTrailKey> TrailKeys;
	/** Samples per second of TrajectoryPoints (= the timeline's frame rate: one point per frame). */
	double TrajectoryRate = 30.0;
	/** One pose per ghost component (outgoing, incoming, outgoing, ...). */
	TArray<TArray<FTransform>> GhostPoses;
};

/** Camera views of the preview viewport (same set as the Unreal viewport's view menu). */
enum class EAnimStitchViewMode : uint8 { Perspective, Front, Back, Left, Right, Top, Bottom };

class FAnimStitchViewportClient : public FEditorViewportClient
{
public:
	/** Switches between the free perspective camera and the axis-locked orthographic views; ortho views are framed on the character. */
	void SetCameraView(EAnimStitchViewMode Mode);
	EAnimStitchViewMode GetCameraView() const { return CameraView; }

	FAnimStitchViewportClient(const TSharedRef<FAnimStitchPreview>& InPreview, const TSharedRef<SEditorViewport>& InViewport);

	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual void MouseMove(FViewport* InViewport, int32 X, int32 Y) override;
	virtual bool InputAxis(const FInputKeyEventArgs& Args) override;
	virtual void CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;

private:
	void UpdateDrag(FViewport* InViewport, int32 X, int32 Y);

	TWeakPtr<FAnimStitchPreview> Preview;
	bool bGizmoDrag = false;
	EAnimStitchViewMode CameraView = EAnimStitchViewMode::Perspective;
	/** The free camera, kept while an orthographic view is active so Perspective restores it. */
	FVector SavedPerspectiveLocation = FVector::ZeroVector;
	FRotator SavedPerspectiveRotation = FRotator::ZeroRotator;
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
	void SetViewMode(EAnimStitchViewMode Mode) { if (Client.IsValid()) { Client->SetCameraView(Mode); } }
	EAnimStitchViewMode GetViewMode() const { return Client.IsValid() ? Client->GetCameraView() : EAnimStitchViewMode::Perspective; }
	/** Lit / Unlit / Wireframe (the engine view mode index). */
	void SetShadingMode(EViewModeIndex Mode) { if (Client.IsValid()) { Client->SetViewMode(Mode); Client->Invalidate(); } }
	EViewModeIndex GetShadingMode() const { return Client.IsValid() ? Client->GetViewMode() : VMI_Lit; }
	void HandleRedraw();
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
protected:
	/** F: the regular viewport's focus command, centring and zooming to the selected control. */
	virtual void OnFocusViewportToSelection() override;
public:

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<FAnimStitchViewportClient> Client;
};







