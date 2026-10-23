// Viewport motion trails (Phase 20): the evaluated path of the selected control, with a point at each of its keys.
// IK targets, pole vectors and the Root motion offset have draggable points (they write the key's translation through the normal
// key-writing path, inside one undo transaction); FK bone tracks are display only because their value is a rotation.
// Kept apart from AnimStitchPreview.cpp / AnimStitchPreviewControls.cpp.

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
	/** Samples per second along the trail (the path is a polyline through these). */
	constexpr double TrailSampleRate = 30.0;
	/** Longest polyline: longer timelines are sampled more sparsely. */
	constexpr int32 MaxTrailSamples = 600;
	/** How close (pixels) the cursor has to be to a key point to grab it. */
	constexpr float TrailPickPixels = 9.f;
}

// The trail always reads the timeline asset's own tracks (the preview's copy is only refreshed on the panel's next tick), so it can never show a deleted or moved key.
const TArray<FAnimStitchTrack>& FAnimStitchPreview::GetTrailTracks() const
{
	const UAnimStitchTimelineAsset* AssetPtr = Asset.Get();
	return AssetPtr ? AssetPtr->Timeline.Tracks : ManualTracks;
}

/** Hash of everything a trail is drawn from: any key added, deleted, moved or edited (by any route, undo / redo included) changes it. */
uint32 FAnimStitchPreview::ComputeTrailSignature() const
{
	uint32 Hash = 0;
	for (const FAnimStitchTrack& Track : GetTrailTracks())
	{
		Hash = HashCombine(Hash, GetTypeHash(Track.Bone));
		Hash = HashCombine(Hash, GetTypeHash((int32)Track.Kind * 16 + (int32)Track.Mode * 4 + (Track.bShowTrail ? 2 : 0) + (Track.bRotationOnly ? 1 : 0)));
		if (!Track.bShowTrail) { continue; }
		Hash = HashCombine(Hash, GetTypeHash(Track.ControlStyle.Offset));
		Hash = HashCombine(Hash, GetTypeHash(Track.PoleStyle.Offset));
		Hash = HashCombine(Hash, GetTypeHash(Track.Keys.Num()));
		for (const FAnimStitchTrackKey& K : Track.Keys)
		{
			Hash = HashCombine(Hash, GetTypeHash(K.Time));
			Hash = HashCombine(Hash, GetTypeHash(K.Translation));
			Hash = HashCombine(Hash, GetTypeHash(K.Pole));
			Hash = HashCombine(Hash, GetTypeHash(K.Scale));
			Hash = HashCombine(Hash, GetTypeHash(K.Rotation.X + 3.0 * K.Rotation.Y + 7.0 * K.Rotation.Z + 11.0 * K.Rotation.W));
			Hash = HashCombine(Hash, GetTypeHash(K.Weight));
			Hash = HashCombine(Hash, GetTypeHash((int32)K.Channels));
		}
	}
	return Hash;
}

void FAnimStitchPreview::EnsureTrail() const
{
	const uint32 Signature = ComputeTrailSignature();
	if (bTrailDirty || Signature != TrailSignature)
	{
		TrailSignature = Signature;
		RebuildTrail();
	}
}

bool FAnimStitchPreview::IsHeavyTrail() const
{
	// FK bone trails need whole poses evaluated; they are not rebuilt while a drag is running.
	return SelectedControl.Kind == EAnimStitchControlKind::Bone && GetTrailTracks().IsValidIndex(SelectedControl.A) && GetTrailTracks()[SelectedControl.A].Kind == EAnimStitchTrackKind::Bone;
}

void FAnimStitchPreview::RebuildTrail() const
{
	bTrailDirty = false;
	TArray<FTrailSegment> OldSegments = MoveTemp(TrailSegments);
	TArray<FTrailKey> OldKeys = MoveTemp(TrailKeys);
	TrailSegments.Reset();
	TrailKeys.Reset();
	if (!IsReady()) { return; }

	// Every control whose track has its trail switched on shows it (G, the master helper switch, is checked when drawing).
	for (int32 TrackIndex = 0; TrackIndex < GetTrailTracks().Num(); ++TrackIndex)
	{
		const FAnimStitchTrack& Track = GetTrailTracks()[TrackIndex];
		if (!Track.bShowTrail || Track.Keys.Num() == 0) { continue; }
		TArray<FAnimStitchControlId, TInlineAllocator<2>> Ids;
		if (Track.Kind == EAnimStitchTrackKind::Bone || Track.Kind == EAnimStitchTrackKind::RootOffset)
		{
			Ids.Add({ EAnimStitchControlKind::Bone, TrackIndex, INDEX_NONE });
		}
		else if (Track.Kind == EAnimStitchTrackKind::IKLimb)
		{
			Ids.Add({ EAnimStitchControlKind::IKEffector, TrackIndex, INDEX_NONE });
			const bool bHasPoleTrack = GetTrailTracks().ContainsByPredicate([&Track](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::IKPole && T.Bone == Track.Bone; });
			if (!bHasPoleTrack) { Ids.Add({ EAnimStitchControlKind::IKPole, TrackIndex, INDEX_NONE }); }
		}
		else if (Track.Kind == EAnimStitchTrackKind::IKPole)
		{
			Ids.Add({ EAnimStitchControlKind::IKPole, TrackIndex, INDEX_NONE });
		}
		for (const FAnimStitchControlId& Id : Ids)
		{
			const bool bHeavy = Id.Kind == EAnimStitchControlKind::Bone && Track.Kind == EAnimStitchTrackKind::Bone;
			if (bHeavy && Drag.bActive)
			{
				// Keep the last result of the expensive FK trails while something is being dragged.
				for (const FTrailSegment& S : OldSegments) { if (S.Control == Id) { TrailSegments.Add(S); } }
				for (const FTrailKey& K : OldKeys) { if (K.Control == Id) { TrailKeys.Add(K); } }
				continue;
			}
			BuildTrailFor(Id);
		}
	}
}

void FAnimStitchPreview::BuildTrailFor(const FAnimStitchControlId& Id) const
{
	const FAnimStitchTrack& Track = GetTrailTracks()[Id.A];
	const FReferenceSkeleton& SkelRef = Evaluator.GetSkeleton()->GetReferenceSkeleton();
	const bool bRootOffset = Track.Kind == EAnimStitchTrackKind::RootOffset;
	const bool bPoleLane = Id.Kind == EAnimStitchControlKind::IKPole && Track.Kind == EAnimStitchTrackKind::IKPole;
	const bool bPoleInLimb = Id.Kind == EAnimStitchControlKind::IKPole && Track.Kind == EAnimStitchTrackKind::IKLimb;
	const bool bEffector = Id.Kind == EAnimStitchControlKind::IKEffector && Track.Kind == EAnimStitchTrackKind::IKLimb;
	const bool bFkBone = Id.Kind == EAnimStitchControlKind::Bone && Track.Kind == EAnimStitchTrackKind::Bone;
	const bool bEditable = bRootOffset || bPoleLane || bPoleInLimb || bEffector;
	if (!bEditable && !bFkBone)
	{
		return;
	}
	// The cube is drawn at value + display offset; the trail follows the cube.
	const FVector DisplayOffset = bPoleInLimb ? Track.PoleStyle.Offset : Track.ControlStyle.Offset;

	// An IK effector's trail is where the foot / hand actually ends up (animation -> IK blend -> animation), so it is one continuous path that joins the animated
	// motion; the key points sit on it. FK bones are traced the same way.
	const bool bTraceBone = bFkBone || bEffector;
	const int32 SkelBone = bTraceBone ? SkelRef.FindBoneIndex(Track.Bone) : INDEX_NONE;
	if (bTraceBone && SkelBone == INDEX_NONE)
	{
		return;
	}
	if (bRootOffset && TrajectoryBase.Num() == 0)
	{
		return;
	}
	if (bTraceBone && !bPlantFeetResolved)
	{
		PlantFeet = FAnimStitchFootLock::ResolveGroundContacts(Evaluator.GetSkeleton(), Evaluator.GetTimeline().FootLock);
		bPlantFeetResolved = true;
	}
	TArray<FTransform> Scratch;
	auto BaseRootAt = [this](double Time) -> FVector
	{
		const double Position = FMath::Clamp(Time * TrajectoryRate, 0.0, double(TrajectoryBase.Num() - 1));
		const int32 I0 = FMath::FloorToInt32(Position);
		const int32 I1 = FMath::Min(I0 + 1, TrajectoryBase.Num() - 1);
		return FMath::Lerp(TrajectoryBase[I0], TrajectoryBase[I1], Position - I0);
	};
	// The value shown at a time; false where the control has no value (IK off / no pole yet).
	auto ValueAt = [&](double Time, FVector& Out) -> bool
	{
		if (bTraceBone)
		{
			if (CorrectedPoses.Num() > 1 && (Evaluator.GetTimeline().FootLock.bEnabled || Evaluator.GetTimeline().bPredictFeet || Evaluator.GetTimeline().bGroundFeet))
			{
				Scratch = CorrectedPoses[FMath::Clamp(FMath::RoundToInt32(Time * CorrectedFps), 0, CorrectedPoses.Num() - 1)];
			}
			else
			{
				Evaluator.EvaluatePose(Time, Scratch);
			}
			FAnimStitchManualEdits::Apply(SkelRef, GetTrailTracks(), Time, Scratch, &PlantFeet);
			Out = FAnimStitchFootLock::GetComponentSpace(SkelRef, Scratch, SkelBone).GetTranslation() + DisplayOffset;
			return true;
		}
		const FAnimStitchTrackKey Key = FAnimStitchManualEdits::Sample(Track, Time);
		if (bRootOffset) { Out = BaseRootAt(Time) + Key.Translation + DisplayOffset; return true; }
		
		if (bPoleLane) { Out = Key.Translation + DisplayOffset; return true; }
		Out = Key.Pole + DisplayOffset;
		return !Key.Pole.IsNearlyZero(1e-3);
	};

	// The path: one sample per frame between the first and last key, thinned out for very long spans.
	const double T0 = Track.Keys[0].Time, T1 = Track.Keys.Last().Time;
	const int32 Wanted = FMath::Max(2, FMath::CeilToInt32((T1 - T0) * TrailSampleRate) + 1);
	// While a point is being dragged the path is sampled more coarsely so it can follow the mouse.
	const int32 Count = FMath::Min(Wanted, Drag.bActive ? 90 : MaxTrailSamples);
	FTrailSegment* Segment = nullptr;
	for (int32 i = 0; i < Count; ++i)
	{
		const double Time = Count > 1 ? FMath::Lerp(T0, T1, double(i) / double(Count - 1)) : T0;
		FVector Value;
		if (ValueAt(Time, Value))
		{
			if (!Segment)
			{
				Segment = &TrailSegments.AddDefaulted_GetRef();
				Segment->Control = Id;
				Segment->bEditable = bEditable;
			}
			Segment->Points.Add(Value);
		}
		else
		{
			Segment = nullptr;
		}
	}
	// The points: exactly at the keys.
	for (int32 KeyIndex = 0; KeyIndex < Track.Keys.Num(); ++KeyIndex)
	{
		FVector Value;
		if (ValueAt(Track.Keys[KeyIndex].Time, Value))
		{
			FTrailKey& Point = TrailKeys.AddDefaulted_GetRef();
			Point.Control = Id;
			Point.bEditable = bEditable;
			Point.KeyIndex = KeyIndex;
			Point.Time = Track.Keys[KeyIndex].Time;
			Point.Pos = Value;
		}
	}
}

bool FAnimStitchPreview::IsTrailEditable() const
{
	EnsureTrail();
	return TrailKeys.ContainsByPredicate([this](const FTrailKey& K) { return K.Control == SelectedControl && K.bEditable; });
}

int32 FAnimStitchPreview::GetNumTrailKeysFor(const FAnimStitchControlId& Id) const
{
	EnsureTrail();
	int32 N = 0;
	for (const FTrailKey& K : TrailKeys) { N += K.Control == Id ? 1 : 0; }
	return bHelpersVisible ? N : 0; // G is the master switch
}

void FAnimStitchPreview::GetTrailGeometry(const FAnimStitchControlId& Id, TArray<TArray<FVector>>& OutSegments, TArray<FVector>& OutKeyPoints) const
{
	EnsureTrail();
	OutSegments.Reset();
	OutKeyPoints.Reset();
	for (const FTrailSegment& S : TrailSegments) { if (S.Control == Id) { OutSegments.Add(S.Points); } }
	for (const FTrailKey& K : TrailKeys) { if (K.Control == Id) { OutKeyPoints.Add(K.Pos); } }
}

int32 FAnimStitchPreview::GetNumTrailKeys() const
{
	return GetNumTrailKeysFor(SelectedControl);
}

int32 FAnimStitchPreview::FindTrailKey(double Time) const
{
	GetNumTrailKeys(); // rebuilds if needed
	return TrailKeys.IndexOfByPredicate([this, Time](const FTrailKey& K) { return K.Control == SelectedControl && FMath::IsNearlyEqual(K.Time, Time, 1e-3); });
}

int32 FAnimStitchPreview::PickTrailKey(const FSceneView& View, const FVector2D& Pixel) const
{
	if (!bHelpersVisible) { return INDEX_NONE; }
	EnsureTrail();
	int32 Best = INDEX_NONE;
	double BestDistance = TrailPickPixels;
	for (int32 i = 0; i < TrailKeys.Num(); ++i)
	{
		FVector2D Screen;
		if (!FSceneView::ProjectWorldToScreen(TrailKeys[i].Pos, View.UnconstrainedViewRect, View.ViewMatrices.GetWorldToClip(), Screen)) { continue; }
		const double Distance = FVector2D::Distance(Screen, Pixel);
		if (Distance < BestDistance) { BestDistance = Distance; Best = i; }
	}
	return Best;
}

void FAnimStitchPreview::DrawTrail(const FSceneView& View, FPrimitiveDrawInterface* PDI) const
{
	if (!bHelpersVisible) { return; } // the G helper toggle hides the trail with everything else
	EnsureTrail();
	if (TrailSegments.Num() == 0 && TrailKeys.Num() == 0) { return; }

	// Draggable trails (IK, pole, root offset) are orange; display-only FK bone trails are cyan.
	auto ColorFor = [](bool bEditable, const FAnimStitchControlId& Id)
	{
		if (Id.Kind == EAnimStitchControlKind::IKPole) { return FLinearColor(0.75f, 0.45f, 1.f); } // pole vector trails are purple, like the pole lane
		return bEditable ? FLinearColor(1.f, 0.55f, 0.1f) : FLinearColor(0.25f, 0.8f, 1.f);
	};
	for (const FTrailSegment& Segment : TrailSegments)
	{
		for (int32 i = 1; i < Segment.Points.Num(); ++i)
		{
			PDI->DrawLine(Segment.Points[i - 1], Segment.Points[i], ColorFor(Segment.bEditable, Segment.Control), SDPG_Foreground, 1.5f, 0.f, true);
		}
	}
	const FVector CamRight = View.GetViewRight();
	const FVector CamUp = View.GetViewUp();
	const float ProjScale = View.ViewMatrices.GetViewToClip().M[0][0];
	const float ViewWidth = float(FMath::Max(1, View.UnscaledViewRect.Width()));
	for (const FTrailKey& Point : TrailKeys)
	{
		const float W = View.WorldToScreen(Point.Pos).W;
		const float Radius = 4.5f * 2.f * FMath::Max(W, 0.01f) / (ViewWidth * FMath::Max(ProjScale, 0.01f));
		const bool bDragged = Drag.bActive && Drag.bTrail && Point.Control == Drag.Control.Id && FMath::IsNearlyEqual(Point.Time, Drag.Time, 1e-6);
		const FLinearColor LineColor = ColorFor(Point.bEditable, Point.Control);
		const FLinearColor DotColor = bDragged ? FLinearColor(1.f, 0.95f, 0.25f) : FLinearColor::White;
		for (const float Fraction : { 1.f, 0.66f, 0.33f })
		{
			DrawCircle(PDI, Point.Pos, CamRight, CamUp, DotColor, Radius * Fraction, 12, SDPG_Foreground, 1.5f, 0.f, true);
		}
		DrawCircle(PDI, Point.Pos, CamRight, CamUp, LineColor, Radius * 1.25f, 12, SDPG_Foreground, 1.5f, 0.f, true);
	}
}

bool FAnimStitchPreview::StartTrailDrag(int32 TrailKey, const FVector& StartHit, const FVector2D& Pixel)
{
	UAnimStitchTimelineAsset* AssetPtr = Asset.Get();
	if (!AssetPtr || Drag.bActive || !TrailKeys.IsValidIndex(TrailKey) || !TrailKeys[TrailKey].bEditable) { return false; }
	const FTrailKey& Point = TrailKeys[TrailKey];
	if (!AssetPtr->Timeline.Tracks.IsValidIndex(Point.Control.A)) { return false; }
	const FAnimStitchTrack& Track = AssetPtr->Timeline.Tracks[Point.Control.A];
	if (!Track.Keys.IsValidIndex(Point.KeyIndex)) { return false; }

	Drag = FDragState();
	Drag.bActive = true;
	Drag.bTrail = true;
	Drag.Axis = 3; // moves in the screen plane
	Drag.Control.Id = Point.Control;
	Drag.Control.Position = Point.Pos;
	Drag.Time = Point.Time;
	Drag.StartKey = Track.Keys[Point.KeyIndex];
	Drag.StartRayHit = StartHit;
	Drag.StartPixel = Pixel;
	DragTransaction = MakeShared<FScopedTransaction>(LOCTEXT("TrailDragTx", "Move Motion Trail Key"));
	DragUpdates = 0;
	DragWrites = 0;
	DragThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
	return true;
}

bool FAnimStitchPreview::DebugDragTrailKey(int32 TrailKey, const FVector& Translation)
{
	EnsureTrail();
	if (!StartTrailDrag(TrailKey, FVector::ZeroVector, FVector2D::ZeroVector)) { return false; }
	Drag.LastTranslation = Translation;
	ApplyControlEdit(Translation, FQuat::Identity, FVector::OneVector);
	EndGizmoDrag();
	return true;
}

#undef LOCTEXT_NAMESPACE
