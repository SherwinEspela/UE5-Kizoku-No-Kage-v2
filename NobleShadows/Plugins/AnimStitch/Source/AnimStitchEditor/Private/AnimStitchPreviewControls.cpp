// Manual correction controls (Phase 9): control shapes in the preview viewport, a translate/rotate/scale gizmo and the
// key-writing that dragging does. Kept apart from AnimStitchPreview.cpp.

#include "AnimStitchPreview.h"
#include "AnimStitchPoseComponent.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchManualEdits.h"
#include "Engine/SkeletalMesh.h"
#include "ScopedTransaction.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "ReferenceSkeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace
{
	constexpr float PickRadiusPixels = 14.f;
	constexpr float HandlePixels = 10.f;
	const FVector AxisDirection[3] = { FVector::ForwardVector, FVector::RightVector, FVector::UpVector };
	const FLinearColor AxisColor[3] = { FLinearColor(0.95f, 0.15f, 0.15f), FLinearColor(0.15f, 0.85f, 0.2f), FLinearColor(0.2f, 0.4f, 1.f) };

	/** Manipulator axes: the world axes, or the control's own orientation in local space. */
	void MakeAxes(const FAnimStitchControl& Control, EAnimStitchCoordSpace Space, FVector Out[3])
	{
		if (Space == EAnimStitchCoordSpace::Local)
		{
			Out[0] = Control.Orientation.GetAxisX(); Out[1] = Control.Orientation.GetAxisY(); Out[2] = Control.Orientation.GetAxisZ();
		}
		else
		{
			Out[0] = AxisDirection[0]; Out[1] = AxisDirection[1]; Out[2] = AxisDirection[2];
		}
	}

	/** Corners of the small plane handle for the plane whose normal is axis N. */
	void PlaneCorners(const FVector& P, const FVector Axes[3], int32 N, double L, FVector Out[4])
	{
		const FVector A = Axes[(N + 1) % 3], B = Axes[(N + 2) % 3];
		const double In = L * 0.28, Out2 = L * 0.52;
		Out[0] = P + A * In + B * In; Out[1] = P + A * Out2 + B * In; Out[2] = P + A * Out2 + B * Out2; Out[3] = P + A * In + B * Out2;
	}

	bool PointInPolygon(const FVector2D& Point, const FVector2D Poly[4])
	{
		bool bInside = false;
		for (int32 i = 0, j = 3; i < 4; j = i++)
		{
			if (((Poly[i].Y > Point.Y) != (Poly[j].Y > Point.Y)) && (Point.X < (Poly[j].X - Poly[i].X) * (Point.Y - Poly[i].Y) / (Poly[j].Y - Poly[i].Y) + Poly[i].X))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	/** Gizmo size in world units that stays roughly constant on screen. */
	double GizmoScale(const FSceneView& View, const FVector& Position)
	{
		const double W = View.WorldToScreen(Position).W;
		const double Width = FMath::Max(1, View.UnconstrainedViewRect.Width());
		const double ProjX = FMath::Max(1e-4, (double)View.ViewMatrices.GetViewToClip().M[0][0]);
		return FMath::Max(1.0, W * 150.0 / (Width * ProjX));
	}

	bool Project(const FSceneView& View, const FVector& World, FVector2D& Out)
	{
		return FSceneView::ProjectWorldToScreen(World, View.UnconstrainedViewRect, View.ViewMatrices.GetWorldToClip(), Out);
	}

	double DistanceToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double Length2 = AB.SizeSquared();
		const double T = Length2 > 1e-6 ? FMath::Clamp(FVector2D::DotProduct(P - A, AB) / Length2, 0.0, 1.0) : 0.0;
		return FVector2D::Distance(P, A + AB * T);
	}

	/** Closest-approach parameter along the line (Origin + t * Dir) to a view ray. */
	bool AxisParameterFromRay(const FVector& Origin, const FVector& Dir, const FVector& RayOrigin, const FVector& RayDir, double& OutT)
	{
		const FVector W0 = Origin - RayOrigin;
		const double B = FVector::DotProduct(Dir, RayDir);
		const double D = FVector::DotProduct(Dir, W0);
		const double E = FVector::DotProduct(RayDir, W0);
		const double Denominator = 1.0 - B * B;
		if (FMath::Abs(Denominator) < 1e-4) { return false; }
		OutT = (B * E - D) / Denominator;
		return true;
	}

	bool RayPlane(const FVector& RayOrigin, const FVector& RayDir, const FVector& PlanePoint, const FVector& PlaneNormal, FVector& OutHit)
	{
		const double Denominator = FVector::DotProduct(RayDir, PlaneNormal);
		if (FMath::Abs(Denominator) < 1e-6) { return false; }
		const double T = FVector::DotProduct(PlanePoint - RayOrigin, PlaneNormal) / Denominator;
		if (T < 0.0) { return false; }
		OutHit = RayOrigin + RayDir * T;
		return true;
	}

	bool Deproject(const FSceneView& View, const FVector2D& Pixel, FVector& Origin, FVector& Direction)
	{
		View.DeprojectFVector2D(Pixel, Origin, Direction);
		Direction.Normalize();
		return true;
	}
}

void FAnimStitchPreview::SetManualTracks(const TArray<FAnimStitchTrack>& Tracks)
{
	ManualTracks = Tracks;
	if (SelectedControl.IsValid() && SelectedControl.Kind != EAnimStitchControlKind::LockPin && !ManualTracks.IsValidIndex(SelectedControl.A))
	{
		SelectControl(FAnimStitchControlId());
	}
	SetPlayhead(Playhead);
}

void FAnimStitchPreview::SetLockInfo(FAnimStitchFootLockResult&& Lock, FAnimStitchPinOutput&& Output, double Fps)
{
	LockResult = MoveTemp(Lock);
	LockOutput = MoveTemp(Output);
	LockFps = Fps;
	bHaveLock = LockResult.bValid;
}

void FAnimStitchPreview::ClearLockInfo()
{
	bHaveLock = false;
	LockResult = FAnimStitchFootLockResult();
	LockOutput = FAnimStitchPinOutput();
}

void FAnimStitchPreview::SelectControl(const FAnimStitchControlId& Id)
{
	if (!(SelectedControl == Id))
	{
		SelectedControl = Id;
		OnControlSelectionChanged.Broadcast();
		RequestRedraw();
	}
}

bool FAnimStitchPreview::GetIKLiveKey(int32 TrackIndex, FAnimStitchTrackKey& OutKey) const
{
	TArray<FAnimStitchControl> Controls;
	BuildControls(Controls);
	bool bEffector = false, bPole = false;
	for (const FAnimStitchControl& C : Controls)
	{
		if (C.Id.A != TrackIndex) { continue; }
		if (C.Id.Kind == EAnimStitchControlKind::IKEffector) { OutKey.Translation = C.Position; OutKey.Rotation = C.Orientation; bEffector = true; }
		if (C.Id.Kind == EAnimStitchControlKind::IKPole) { OutKey.Pole = C.Position; bPole = true; }
	}
	return bEffector && bPole;
}

void FAnimStitchPreview::BuildControls(TArray<FAnimStitchControl>& Out) const
{
	BuildControlsFromPose(Out);
	if (Drag.bActive)
	{
		// While dragging, the control follows the drag itself (not the pose), so it moves with the cursor every frame.
		for (FAnimStitchControl& C : Out)
		{
			if (C.Id == Drag.Control.Id && C.Id.Kind != EAnimStitchControlKind::LockPin)
			{
				C.Position = Drag.Control.Position + Drag.LastTranslation;
				C.Orientation = (Drag.LastRotation * Drag.Control.Orientation).GetNormalized();
			}
		}
	}
}

void FAnimStitchPreview::BuildControlsFromPose(TArray<FAnimStitchControl>& Out) const
{
	Out.Reset();
	if (!MainComponent || !Mesh) { return; }
	const TArray<FTransform>& Component = MainComponent->GetComponentSpaceTransforms();
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	if (Component.Num() != Ref.GetNum()) { return; }

	for (int32 TrackIndex = 0; TrackIndex < ManualTracks.Num(); ++TrackIndex)
	{
		const FAnimStitchTrack& Track = ManualTracks[TrackIndex];
		const int32 Bone = Ref.FindBoneIndex(Track.Bone);
		if (Bone == INDEX_NONE) { continue; }

		if (Track.Kind == EAnimStitchTrackKind::Bone)
		{
			FAnimStitchControl C;
			C.Id = { EAnimStitchControlKind::Bone, TrackIndex, INDEX_NONE };
			C.Position = Component[Bone].GetTranslation();
			C.Orientation = Component[Bone].GetRotation();
			C.bTranslate = !Track.bRotationOnly;
			C.bScale = !Track.bRotationOnly;
			C.Color = Track.bRotationOnly ? FLinearColor(0.9f, 0.6f, 0.2f) : FLinearColor(0.4f, 0.9f, 0.5f);
				C.Style = Track.bRotationOnly ? 1 : 0;
			C.Label = Track.Bone.ToString();
			Out.Add(C);
		}
		else
		{
			const FAnimStitchFootChain Chain = FAnimStitchFootLock::BuildChain(Ref, Track.Bone);
			if (!Chain.IsValid()) { continue; }
			const FAnimStitchTrackKey Key = FAnimStitchManualEdits::Sample(Track, Playhead);
			const bool bActive = Track.Keys.Num() > 0 && Key.Weight > 1e-3f;

			FAnimStitchControl Effector;
			Effector.Id = { EAnimStitchControlKind::IKEffector, TrackIndex, INDEX_NONE };
			Effector.Position = bActive ? Key.Translation : Component[Chain.Foot].GetTranslation();
			Effector.Orientation = bActive ? Key.Rotation : Component[Chain.Foot].GetRotation();
			Effector.bScale = false;
			Effector.Color = FLinearColor(1.f, 0.5f, 0.15f);
				Effector.Style = 2;
			Effector.Label = Track.Bone.ToString() + TEXT(" IK");
			Out.Add(Effector);

			FAnimStitchControl Pole;
			Pole.Id = { EAnimStitchControlKind::IKPole, TrackIndex, INDEX_NONE };
			Pole.Position = (bActive && !Key.Pole.IsNearlyZero(1e-3)) ? Key.Pole : FAnimStitchManualEdits::DefaultPoleForPose(Ref, Chain, Component);
			Pole.bRotate = false;
			Pole.bScale = false;
			Pole.Color = FLinearColor(0.7f, 0.4f, 1.f);
				Pole.Style = 3;
			Pole.Label = Track.Bone.ToString() + TEXT(" pole");
			Out.Add(Pole);
		}
	}

	if (bHaveLock)
	{
		const int32 Frame = FMath::RoundToInt32(Playhead * LockFps);
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			if (LockResult.Mask[Foot].IsValidIndex(Frame) && LockResult.Mask[Foot][Frame] == AnimStitchPlant::ManualPin && LockOutput.ManualTarget[Foot].IsValidIndex(Frame))
			{
				FAnimStitchControl Pin;
				Pin.Id = { EAnimStitchControlKind::LockPin, Foot, Frame };
				Pin.Position = LockOutput.ManualTarget[Foot][Frame];
				Pin.bRotate = false;
				Pin.bScale = false;
				Pin.Color = FLinearColor(0.3f, 1.f, 0.5f);
					Pin.Style = 4;
				Pin.Label = Foot == 0 ? TEXT("L foot lock pin") : TEXT("R foot lock pin");
				Out.Add(Pin);
			}
		}
	}
}

namespace
{
	/** Half-size of a control cube in world units. */
	double ControlHalfSize(const FSceneView& View, const FAnimStitchControl& C, float DrawSize, bool bSelected)
	{
		const double Base = GizmoScale(View, C.Position) * 0.16 * DrawSize * (bSelected ? 1.25 : 1.0);
		return C.Id.Kind == EAnimStitchControlKind::IKEffector ? Base * 1.3 : Base;
	}
}

FAnimStitchControlId FAnimStitchPreview::PickControl(const FSceneView& View, const FVector2D& Pixel) const
{
	if (!bShowControls) { return FAnimStitchControlId(); }
	TArray<FAnimStitchControl> Controls;
	BuildControls(Controls);
	FAnimStitchControlId Best;
	double BestDistance = TNumericLimits<double>::Max();
	for (const FAnimStitchControl& C : Controls)
	{
		const double Half = ControlHalfSize(View, C, ControlDrawSize, C.Id == SelectedControl);
		FVector2D Min(TNumericLimits<double>::Max(), TNumericLimits<double>::Max()), Max(-TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());
		bool bAll = true;
		for (int32 Corner = 0; Corner < 8 && bAll; ++Corner)
		{
			FVector2D Screen;
			bAll = Project(View, C.Position + FVector((Corner & 1) ? Half : -Half, (Corner & 2) ? Half : -Half, (Corner & 4) ? Half : -Half), Screen);
			Min.X = FMath::Min(Min.X, Screen.X); Min.Y = FMath::Min(Min.Y, Screen.Y);
			Max.X = FMath::Max(Max.X, Screen.X); Max.Y = FMath::Max(Max.Y, Screen.Y);
		}
		if (!bAll) { continue; }
		constexpr double Pad = 4.0; // a little forgiveness around the cube
		if (Pixel.X < Min.X - Pad || Pixel.X > Max.X + Pad || Pixel.Y < Min.Y - Pad || Pixel.Y > Max.Y + Pad) { continue; }
		const double Distance = FVector2D::Distance((Min + Max) * 0.5, Pixel);
		if (Distance < BestDistance) { BestDistance = Distance; Best = C.Id; }
	}
	return Best;
}
void FAnimStitchPreview::DrawControls(const FSceneView& View, FPrimitiveDrawInterface* PDI) const
{
	if (!bShowControls) { return; }
	TArray<FAnimStitchControl> Controls;
	BuildControls(Controls);
	if (Controls.Num() == 0) { return; }

	for (const FAnimStitchControl& C : Controls)
	{
		const bool bSelected = C.Id == SelectedControl;
		const double Half = ControlHalfSize(View, C, ControlDrawSize, bSelected);
		const FLinearColor Color = bSelected ? FLinearColor(1.f, 0.9f, 0.2f) : C.Color;
		const int32 MaterialIndex = bSelected ? 5 : FMath::Clamp(C.Style, 0, 4);
		if (ControlMaterials[MaterialIndex])
		{
			// Solid, semi-transparent cube (easy to see and to hit), plus crisp edges.
			DrawBox(PDI, FTranslationMatrix(C.Position), FVector(Half), ControlMaterials[MaterialIndex]->GetRenderProxy(), SDPG_Foreground);
		}
		DrawWireBox(PDI, FBox::BuildAABB(C.Position, FVector(Half)), Color, SDPG_Foreground);
		if (C.Id.Kind == EAnimStitchControlKind::IKPole && MainComponent && Mesh && ManualTracks.IsValidIndex(C.Id.A))
		{
			const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
			const FAnimStitchFootChain Chain = FAnimStitchFootLock::BuildChain(Ref, ManualTracks[C.Id.A].Bone);
			if (Chain.IsValid())
			{
				DrawDashedLine(PDI, MainComponent->GetComponentSpaceTransforms()[Chain.Calf].GetTranslation(), C.Position, FLinearColor(0.7f, 0.4f, 1.f), 6.0, SDPG_Foreground);
			}
		}
	}

	// Modern-style manipulator on the selected control: thin long axes with small cone tips, plane handles, thin rings, cube scale tips.
	const FAnimStitchControl* Selected = Controls.FindByPredicate([this](const FAnimStitchControl& C) { return C.Id == SelectedControl; });
	if (!Selected) { return; }
	const FVector P = Selected->Position;
	const double L = GizmoScale(View, P) * GizmoDrawSize;
	EAnimStitchGizmoMode Mode = GizmoMode;
	if (Mode == EAnimStitchGizmoMode::Translate && !Selected->bTranslate) { Mode = EAnimStitchGizmoMode::Rotate; }
	if (Mode == EAnimStitchGizmoMode::Rotate && !Selected->bRotate) { Mode = EAnimStitchGizmoMode::Translate; }
	if (Mode == EAnimStitchGizmoMode::Scale && !Selected->bScale) { Mode = Selected->bTranslate ? EAnimStitchGizmoMode::Translate : EAnimStitchGizmoMode::Rotate; }
	FVector Axes[3];
	MakeAxes(*Selected, CoordSpaces[(int32)Mode], Axes);
	if (Drag.bActive) { for (int32 i = 0; i < 3; ++i) { Axes[i] = Drag.Axes[i]; } }

	const FLinearColor Active(1.f, 0.95f, 0.25f);
	// Thick lines (screen-space pixels) so the manipulator reads clearly.
	constexpr float AxisPx = 3.5f, RingPx = 3.f, PlanePx = 2.5f;
	auto Thick = [PDI](const FVector& A, const FVector& B, const FLinearColor& C, float Px) { PDI->DrawLine(A, B, C, SDPG_Foreground, Px, 0.f, true); };
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const bool bActive = Drag.bActive && (Drag.Axis == Axis || Drag.Axis == 3 || (Drag.Axis >= 4 && Drag.Axis - 4 != Axis));
		const FLinearColor Color = bActive ? Active : AxisColor[Axis];
		const FVector Direction = Axes[Axis];
		if (Mode == EAnimStitchGizmoMode::Rotate)
		{
			const FLinearColor RingColor = (Drag.bActive && Drag.Axis == Axis) ? Active : AxisColor[Axis];
			const FVector U = Axes[(Axis + 1) % 3], V = Axes[(Axis + 2) % 3];
			FVector Previous = P + U * L * 0.8;
			for (int32 i = 1; i <= 64; ++i)
			{
				const double Angle = 2.0 * PI * i / 64.0;
				const FVector Next = P + (U * FMath::Cos(Angle) + V * FMath::Sin(Angle)) * L * 0.8;
				Thick(Previous, Next, RingColor, RingPx);
				Previous = Next;
			}
			continue;
		}
		Thick(P + Direction * L * 0.1, P + Direction * L, Color, AxisPx);
		if (Mode == EAnimStitchGizmoMode::Translate)
		{
			// Cone tip: a ring of lines meeting at the end of the axis.
			const FVector Base = P + Direction * L * 0.8, Tip = P + Direction * L * 1.06;
			const FVector S0 = Axes[(Axis + 1) % 3], S1 = Axes[(Axis + 2) % 3];
			for (int32 k = 0; k < 6; ++k)
			{
				const double Angle = 2.0 * PI * k / 6.0;
				const FVector Rim = Base + (S0 * FMath::Cos(Angle) + S1 * FMath::Sin(Angle)) * L * 0.05;
				Thick(Rim, Tip, Color, 2.f);
			}
			FVector Corners[4];
			PlaneCorners(P, Axes, Axis, L, Corners);
			const FLinearColor PlaneColor = (Drag.bActive && Drag.Axis == 4 + Axis) ? Active : AxisColor[Axis] * 0.9f;
			for (int32 k = 0; k < 4; ++k) { Thick(Corners[k], Corners[(k + 1) % 4], PlaneColor, PlanePx); }
			Thick(Corners[0], Corners[2], PlaneColor * 0.6f, 1.5f);
		}
		else
		{
			DrawWireBox(PDI, FBox::BuildAABB(P + Direction * L, FVector(L * 0.04)), Color, SDPG_Foreground, 3.f, 0.f, true);
		}
	}
	if (Mode != EAnimStitchGizmoMode::Rotate)
	{
		DrawWireBox(PDI, FBox::BuildAABB(P, FVector(L * 0.045)), (Drag.bActive && Drag.Axis == 3) ? Active : FLinearColor::White, SDPG_Foreground, 3.f, 0.f, true);
	}
}
bool FAnimStitchPreview::BeginGizmoDrag(const FSceneView& View, const FVector2D& Pixel)
{
	if (!bShowControls) { return false; }
	TArray<FAnimStitchControl> Controls;
	BuildControls(Controls);
	const FAnimStitchControl* Selected = Controls.FindByPredicate([this](const FAnimStitchControl& C) { return C.Id == SelectedControl; });
	if (!Selected || Drag.bActive) { return false; }

	EAnimStitchGizmoMode Mode = GizmoMode;
	if (Mode == EAnimStitchGizmoMode::Translate && !Selected->bTranslate) { Mode = EAnimStitchGizmoMode::Rotate; }
	if (Mode == EAnimStitchGizmoMode::Rotate && !Selected->bRotate) { Mode = EAnimStitchGizmoMode::Translate; }
	if (Mode == EAnimStitchGizmoMode::Scale && !Selected->bScale) { Mode = Selected->bTranslate ? EAnimStitchGizmoMode::Translate : EAnimStitchGizmoMode::Rotate; }

	const FVector P = Selected->Position;
	const double L = GizmoScale(View, P) * GizmoDrawSize;
	FVector Axes[3];
	MakeAxes(*Selected, CoordSpaces[(int32)Mode], Axes);
	FVector2D Center;
	if (!Project(View, P, Center)) { return false; }

	// Which handle is under the cursor?
	int32 HitAxis = INDEX_NONE;
	double Best = HandlePixels;
	if (Mode == EAnimStitchGizmoMode::Rotate)
	{
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			FVector2D Prev;
			bool bHavePrev = false;
			for (int32 i = 0; i <= 64; ++i)
			{
				const double Angle = 2.0 * PI * i / 64.0;
				FVector2D Screen;
				if (!Project(View, P + (Axes[(Axis + 1) % 3] * FMath::Cos(Angle) + Axes[(Axis + 2) % 3] * FMath::Sin(Angle)) * L * 0.8, Screen)) { bHavePrev = false; continue; }
				if (bHavePrev)
				{
					const double Distance = DistanceToSegment(Pixel, Prev, Screen);
					if (Distance < Best) { Best = Distance; HitAxis = Axis; }
				}
				Prev = Screen;
				bHavePrev = true;
			}
		}
	}
	else
	{
		if (Mode == EAnimStitchGizmoMode::Translate)
		{
			// Plane squares first: they sit away from the axes.
			for (int32 N = 0; N < 3; ++N)
			{
				FVector Corners[4];
				PlaneCorners(P, Axes, N, L, Corners);
				FVector2D Poly[4];
				bool bAll = true;
				for (int32 k = 0; k < 4; ++k) { bAll &= Project(View, Corners[k], Poly[k]); }
				if (bAll && PointInPolygon(Pixel, Poly)) { HitAxis = 4 + N; Best = 0.0; }
			}
		}
		for (int32 Axis = 0; Axis < 3 && HitAxis == INDEX_NONE; ++Axis)
		{
			FVector2D Tip;
			if (!Project(View, P + Axes[Axis] * L, Tip)) { continue; }
			const double Distance = DistanceToSegment(Pixel, Center, Tip);
			if (Distance < Best) { Best = Distance; HitAxis = Axis; }
		}
		if (HitAxis == INDEX_NONE && FVector2D::Distance(Pixel, Center) < HandlePixels + 3.f)
		{
			HitAxis = 3; // centre handle: move in the screen plane / scale uniformly
		}
	}
	if (HitAxis == INDEX_NONE) { return false; }

	// Start values.
	UAnimStitchTimelineAsset* AssetPtr = Asset.Get();
	if (!AssetPtr) { return false; }
	Drag = FDragState();
	Drag.bActive = true;
	Drag.Axis = HitAxis;
	Drag.Control = *Selected;
	Drag.Time = Playhead;
	Drag.StartPixel = Pixel;
	Drag.LastTranslation = FVector::ZeroVector;
	for (int32 i = 0; i < 3; ++i) { Drag.Axes[i] = Axes[i]; }

	FVector RayOrigin, RayDir;
	Deproject(View, Pixel, RayOrigin, RayDir);
	const FVector CameraForward = View.GetViewDirection();
	if (Mode == EAnimStitchGizmoMode::Rotate)
	{
		FVector Hit;
		if (!RayPlane(RayOrigin, RayDir, P, Axes[HitAxis], Hit)) { Drag.bActive = false; return false; }
		Drag.StartVector = (Hit - P).GetSafeNormal();
	}
	else if (HitAxis == 3 || HitAxis >= 4)
	{
		FVector Hit;
		const FVector Normal = HitAxis == 3 ? CameraForward : Axes[HitAxis - 4];
		if (!RayPlane(RayOrigin, RayDir, P, Normal, Hit)) { Drag.bActive = false; return false; }
		Drag.StartRayHit = Hit;
	}
	else if (!AxisParameterFromRay(P, Axes[HitAxis], RayOrigin, RayDir, Drag.StartAxisT))
	{
		Drag.bActive = false;
		return false;
	}

	// Remember what the key / pin looked like before the drag, and open one undo transaction for the whole drag.
	if (Selected->Id.Kind == EAnimStitchControlKind::LockPin)
	{
		const FIntPoint Owner = LockResult.PinEvent[Selected->Id.A].IsValidIndex(Selected->Id.B) ? LockResult.PinEvent[Selected->Id.A][Selected->Id.B] : FIntPoint(-1, -1);
		if (!AssetPtr->Timeline.Clips.IsValidIndex(Owner.X) || !AssetPtr->Timeline.Clips[Owner.X].FootEvents.IsValidIndex(Owner.Y)) { Drag.bActive = false; return false; }
		Drag.StartOffset = AssetPtr->Timeline.Clips[Owner.X].FootEvents[Owner.Y].PinOffset;
	}
	else if (ManualTracks.IsValidIndex(Selected->Id.A))
	{
		Drag.StartKey = FAnimStitchManualEdits::Sample(ManualTracks[Selected->Id.A], Playhead);
		Drag.StartKey.Time = Playhead;
		if (Selected->Id.Kind != EAnimStitchControlKind::Bone)
		{
			// IK: what is on screen is the starting point (the key's target when IK is on, the animated limb otherwise).
			const FAnimStitchTrack& Track = ManualTracks[Selected->Id.A];
			const FAnimStitchFootChain Chain = FAnimStitchFootLock::BuildChain(Mesh->GetRefSkeleton(), Track.Bone);
			const TArray<FTransform>& Component = MainComponent->GetComponentSpaceTransforms();
			const bool bActive = Drag.StartKey.Weight > 1e-3f;
			if (!bActive)
			{
				Drag.StartKey.Translation = Component[Chain.Foot].GetTranslation();
				Drag.StartKey.Rotation = Component[Chain.Foot].GetRotation();
				for (const FAnimStitchControl& C : Controls) { if (C.Id.Kind == EAnimStitchControlKind::IKPole && C.Id.A == Selected->Id.A) { Drag.StartKey.Pole = C.Position; } }
			}
			Drag.StartKey.Weight = 1.f;
		}
	}
	DragTransaction = MakeShared<FScopedTransaction>(LOCTEXT("GizmoDragTx", "Edit Bone Correction"));
	return true;
}

void FAnimStitchPreview::UpdateGizmoDrag(const FSceneView& View, const FVector2D& Pixel)
{
	if (!Drag.bActive) { return; }
	const FAnimStitchControl& C = Drag.Control;
	const FVector P = C.Position;
	EAnimStitchGizmoMode Mode = GizmoMode;
	if (Mode == EAnimStitchGizmoMode::Translate && !C.bTranslate) { Mode = EAnimStitchGizmoMode::Rotate; }
	if (Mode == EAnimStitchGizmoMode::Rotate && !C.bRotate) { Mode = EAnimStitchGizmoMode::Translate; }
	if (Mode == EAnimStitchGizmoMode::Scale && !C.bScale) { Mode = C.bTranslate ? EAnimStitchGizmoMode::Translate : EAnimStitchGizmoMode::Rotate; }

	FVector RayOrigin, RayDir;
	Deproject(View, Pixel, RayOrigin, RayDir);

	FVector Translation = FVector::ZeroVector;
	FQuat Rotation = FQuat::Identity;
	FVector Scale = FVector::OneVector;

	if (Mode == EAnimStitchGizmoMode::Translate)
	{
		if (Drag.Axis == 3 || Drag.Axis >= 4)
		{
			FVector Hit;
			const FVector Normal = Drag.Axis == 3 ? View.GetViewDirection() : Drag.Axes[Drag.Axis - 4];
			if (RayPlane(RayOrigin, RayDir, Drag.StartRayHit, Normal, Hit)) { Translation = Hit - Drag.StartRayHit; }
		}
		else
		{
			double T;
			if (AxisParameterFromRay(P, Drag.Axes[Drag.Axis], RayOrigin, RayDir, T)) { Translation = Drag.Axes[Drag.Axis] * (T - Drag.StartAxisT); }
		}
	}
	else if (Mode == EAnimStitchGizmoMode::Rotate)
	{
		FVector Hit;
		if (RayPlane(RayOrigin, RayDir, P, Drag.Axes[Drag.Axis], Hit))
		{
			const FVector Now = (Hit - P).GetSafeNormal();
			const double Angle = FMath::Atan2(FVector::DotProduct(Drag.Axes[Drag.Axis], FVector::CrossProduct(Drag.StartVector, Now)), FVector::DotProduct(Drag.StartVector, Now));
			Rotation = FQuat(Drag.Axes[Drag.Axis], Angle);
		}
	}
	else // Scale
	{
		if (Drag.Axis == 3)
		{
			const double Factor = FMath::Exp((Drag.StartPixel.Y - Pixel.Y) * 0.01);
			Scale = FVector(Factor);
		}
		else if (Drag.Axis < 3)
		{
			double T;
			if (AxisParameterFromRay(P, Drag.Axes[Drag.Axis], RayOrigin, RayDir, T))
			{
				const double L = GizmoScale(View, P) * GizmoDrawSize;
				Scale[Drag.Axis] = FMath::Max(0.05, 1.0 + (T - Drag.StartAxisT) / L);
			}
		}
	}
	Drag.LastTranslation = Translation;
	Drag.LastRotation = Rotation;
	ApplyControlEdit(Translation, Rotation, Scale);
}
void FAnimStitchPreview::EndGizmoDrag()
{
	Drag.bActive = false;
	DragTransaction.Reset(); // closes the undo transaction
}

void FAnimStitchPreview::ApplyControlEdit(const FVector& Translation, const FQuat& Rotation, const FVector& Scale)
{
	UAnimStitchTimelineAsset* AssetPtr = Asset.Get();
	if (!AssetPtr || !Drag.bActive) { return; }
	const FAnimStitchControlId Id = Drag.Control.Id;

	if (Id.Kind == EAnimStitchControlKind::LockPin)
	{
		const FIntPoint Owner = LockResult.PinEvent[Id.A].IsValidIndex(Id.B) ? LockResult.PinEvent[Id.A][Id.B] : FIntPoint(-1, -1);
		AssetPtr->SetPinOffset(Owner.X, Owner.Y, Drag.StartOffset + Translation);
		return;
	}
	if (!AssetPtr->Timeline.Tracks.IsValidIndex(Id.A)) { return; }

	FAnimStitchTrackKey Key = Drag.StartKey;
	Key.Time = Drag.Time;
	if (Id.Kind == EAnimStitchControlKind::Bone)
	{
		Key.Translation = Drag.StartKey.Translation + Translation;
		Key.Rotation = (Rotation * Drag.StartKey.Rotation).GetNormalized();
		Key.Scale = Drag.StartKey.Scale * Scale;
	}
	else if (Id.Kind == EAnimStitchControlKind::IKEffector)
	{
		Key.Translation = Drag.StartKey.Translation + Translation;
		Key.Rotation = (Rotation * Drag.StartKey.Rotation).GetNormalized();
		if (Drag.StartKey.Weight < 1e-3f) { Key.Weight = 1.f; }
	}
	else if (Id.Kind == EAnimStitchControlKind::IKPole)
	{
		Key.Pole = Drag.StartKey.Pole + Translation;
	}
	AssetPtr->SetTrackKey(Id.A, Key);
	// Show the result right now (the panel would only pick the change up on its next tick).
	ManualTracks = AssetPtr->Timeline.Tracks;
	SetPlayhead(Playhead);
}

#undef LOCTEXT_NAMESPACE


