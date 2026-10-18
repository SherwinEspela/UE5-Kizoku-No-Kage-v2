#include "SAnimStitchTimeline.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Rendering/DrawElements.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace AnimStitchTimelineUI
{
	const FLinearColor Background(0.04f, 0.04f, 0.045f);
	const FLinearColor RulerBg(0.08f, 0.08f, 0.09f);
	const FLinearColor ClipColors[2] = { FLinearColor(0.16f, 0.34f, 0.55f), FLinearColor(0.20f, 0.46f, 0.34f) };
	const FLinearColor OverlapColor(0.95f, 0.65f, 0.15f, 0.55f);
	const FLinearColor Handle(1.f, 1.f, 1.f, 0.35f);
	const FLinearColor Selected(1.f, 1.f, 1.f, 1.f);

	void DrawRect(FSlateWindowElementList& List, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FVector2f& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(List, Layer, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Color);
	}

	void DrawLine(FSlateWindowElementList& List, int32 Layer, const FGeometry& G, const TArray<FVector2f>& Points, const FLinearColor& Color, float Thickness = 1.f)
	{
		FSlateDrawElement::MakeLines(List, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
	}

	void PaintLabel(FSlateWindowElementList& List, int32 Layer, const FGeometry& G, const FVector2f& Pos, const FString& Str, const FLinearColor& Color)
	{
		static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 9);
		FSlateDrawElement::MakeText(List, Layer, G.ToPaintGeometry(G.GetLocalSize(), FSlateLayoutTransform(Pos)), Str, Font, ESlateDrawEffect::None, Color);
	}
}

void SAnimStitchTimeline::Construct(const FArguments& InArgs)
{
	Asset = InArgs._TimelineAsset;
	OnSelectionChangedDelegate = InArgs._OnSelectionChanged;
	OnPlayheadChangedDelegate = InArgs._OnPlayheadChanged;
	OnWarningsDelegate = InArgs._OnWarnings;
	ForceVolatile(true); // painted from live model data, no invalidation bookkeeping needed
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

SAnimStitchTimeline::~SAnimStitchTimeline()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

// ---------------------------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------------------------

int32 SAnimStitchTimeline::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	using namespace AnimStitchTimelineUI;
	const FVector2f Size = G.GetLocalSize();
	const FAnimStitchTimeline& TL = Asset->Timeline;

	Out.PushClip(FSlateClippingZone(G));
	DrawRect(Out, Layer, G, FVector2f::ZeroVector, Size, Background);
	DrawRect(Out, Layer + 1, G, FVector2f::ZeroVector, FVector2f(Size.X, RulerHeight), RulerBg);

	// Ruler ticks
	static const double Steps[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0 };
	double Step = Steps[UE_ARRAY_COUNT(Steps) - 1];
	for (double S : Steps)
	{
		if (S * PxPerSec >= 70.0)
		{
			Step = S;
			break;
		}
	}
	for (double T = FMath::CeilToDouble(XToTime(0.f) / Step) * Step; TimeToX(T) < Size.X; T += Step)
	{
		const float X = TimeToX(T);
		DrawLine(Out, Layer + 2, G, { FVector2f(X, RulerHeight - 8.f), FVector2f(X, RulerHeight) }, FLinearColor(1, 1, 1, 0.6f));
		PaintLabel(Out, Layer + 2, G, FVector2f(X + 3.f, 4.f), Step >= 1.0 ? FString::Printf(TEXT("%.0fs"), T) : FString::Printf(TEXT("%.2fs"), T), FLinearColor(1, 1, 1, 0.65f));
		if (T > 1e6) { break; }
	}

	// Clips
	for (int32 i = 0; i < TL.Clips.Num(); ++i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		const float X0 = TimeToX(Clip.StartTime);
		const float X1 = TimeToX(Clip.GetEndTime());
		const FVector2f Pos(X0, TrackTop);
		const FVector2f BoxSize(FMath::Max(1.f, X1 - X0), TrackHeight);

		FLinearColor Color = ClipColors[i & 1];
		DrawRect(Out, Layer + 3, G, Pos, BoxSize, Color);
		DrawRect(Out, Layer + 4, G, Pos, FVector2f(FMath::Min(HandleWidth, BoxSize.X * 0.5f), TrackHeight), Handle);
		DrawRect(Out, Layer + 4, G, FVector2f(X1 - FMath::Min(HandleWidth, BoxSize.X * 0.5f), TrackTop), FVector2f(FMath::Min(HandleWidth, BoxSize.X * 0.5f), TrackHeight), Handle);

		const FGeometry Child = G.MakeChild(BoxSize, FSlateLayoutTransform(Pos));
		Out.PushClip(FSlateClippingZone(Child));
		PaintLabel(Out, Layer + 6, G, Pos + FVector2f(HandleWidth + 3.f, 6.f), Clip.Sequence ? Clip.Sequence->GetName() : FString(TEXT("<missing>")), FLinearColor::White);
		PaintLabel(Out, Layer + 6, G, Pos + FVector2f(HandleWidth + 3.f, 24.f), FString::Printf(TEXT("%.2fs"), Clip.GetDuration()), FLinearColor(1, 1, 1, 0.75f));
		Out.PopClip();

		if (i == SelectedClip && !bOverlapSelected)
		{
			DrawLine(Out, Layer + 8, G, { Pos, Pos + FVector2f(BoxSize.X, 0), Pos + BoxSize, Pos + FVector2f(0, BoxSize.Y), Pos }, Selected, 2.f);
		}
	}

	// Overlaps (crossfades)
	for (int32 i = 1; i < TL.Clips.Num(); ++i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		if (Clip.OverlapIn <= 0.0)
		{
			continue;
		}
		const float X0 = TimeToX(Clip.StartTime);
		const float X1 = TimeToX(Clip.StartTime + Clip.OverlapIn);
		const float W = FMath::Max(1.f, X1 - X0);
		DrawRect(Out, Layer + 9, G, FVector2f(X0, TrackTop), FVector2f(W, TrackHeight), OverlapColor);

		// Weight curves: outgoing falls, incoming rises.
		TArray<FVector2f> Fall, Rise;
		constexpr int32 N = 16;
		for (int32 k = 0; k <= N; ++k)
		{
			const float A = float(k) / N;
			const float Wt = Clip.BlendIn == EAnimStitchBlendCurve::Smoothstep ? A * A * (3.f - 2.f * A) : A;
			Rise.Add(FVector2f(X0 + W * A, TrackTop + TrackHeight - 4.f - (TrackHeight - 8.f) * Wt));
			Fall.Add(FVector2f(X0 + W * A, TrackTop + 4.f + (TrackHeight - 8.f) * Wt));
		}
		DrawLine(Out, Layer + 10, G, Rise, FLinearColor::White, 1.5f);
		DrawLine(Out, Layer + 10, G, Fall, FLinearColor(1, 1, 1, 0.6f), 1.5f);
		if (W > 36.f)
		{
			PaintLabel(Out, Layer + 11, G, FVector2f(X0 + 3.f, TrackTop + TrackHeight - 16.f), FString::Printf(TEXT("%.2fs"), Clip.OverlapIn), FLinearColor::White);
		}
		if (i == SelectedClip && bOverlapSelected)
		{
			const FVector2f P(X0, TrackTop), S(W, TrackHeight);
			DrawLine(Out, Layer + 12, G, { P, P + FVector2f(S.X, 0), P + S, P + FVector2f(0, S.Y), P }, Selected, 2.f);
		}
	}

	// Playback range: dim everything outside it and draw start (green) / end (red) markers on the ruler.
	{
		double RangeStart, RangeEnd;
		Asset->GetPlaybackRange(RangeStart, RangeEnd);
		const float SX = TimeToX(RangeStart);
		const float EX = TimeToX(RangeEnd);
		const FLinearColor Dim(0.f, 0.f, 0.f, 0.45f);
		if (SX > 0.f)
		{
			DrawRect(Out, Layer + 13, G, FVector2f(0.f, RulerHeight), FVector2f(FMath::Min(SX, Size.X), Size.Y - RulerHeight), Dim);
		}
		if (EX < Size.X)
		{
			const float X = FMath::Max(EX, 0.f);
			DrawRect(Out, Layer + 13, G, FVector2f(X, RulerHeight), FVector2f(Size.X - X, Size.Y - RulerHeight), Dim);
		}
		const FLinearColor StartColor(0.2f, 0.85f, 0.3f), EndColor(0.95f, 0.25f, 0.25f);
		DrawLine(Out, Layer + 14, G, { FVector2f(SX, 0.f), FVector2f(SX, Size.Y) }, StartColor, 1.f);
		DrawLine(Out, Layer + 14, G, { FVector2f(EX, 0.f), FVector2f(EX, Size.Y) }, EndColor, 1.f);
		// Flags: start points right, end points left.
		DrawRect(Out, Layer + 14, G, FVector2f(SX, 0.f), FVector2f(10.f, 12.f), StartColor);
		DrawRect(Out, Layer + 14, G, FVector2f(EX - 10.f, 0.f), FVector2f(10.f, 12.f), EndColor);
	}

	// Drop insertion marker
	if (bDropHover)
	{
		const float X = TimeToX(DropTime);
		DrawLine(Out, Layer + 13, G, { FVector2f(X, RulerHeight), FVector2f(X, Size.Y) }, FLinearColor(0.3f, 1.f, 0.4f), 2.f);
	}
	if (TL.Clips.Num() == 0)
	{
		PaintLabel(Out, Layer + 13, G, FVector2f(14.f, TrackTop + 22.f), LOCTEXT("EmptyHint", "Drag animation sequences from the Content Browser here").ToString(), FLinearColor(1, 1, 1, 0.5f));
	}

	// Playhead
	const float PX = TimeToX(Playhead);
	DrawLine(Out, Layer + 14, G, { FVector2f(PX, 0.f), FVector2f(PX, Size.Y) }, FLinearColor(1.f, 0.25f, 0.25f), 1.5f);

	Out.PopClip();
	return Layer + 15;
}

// ---------------------------------------------------------------------------------------------
// Hit testing & selection
// ---------------------------------------------------------------------------------------------

SAnimStitchTimeline::FHit SAnimStitchTimeline::HitTest(const FVector2f& P) const
{
	FHit Result;
	if (P.Y < RulerHeight)
	{
		double RangeStart, RangeEnd;
		Asset->GetPlaybackRange(RangeStart, RangeEnd);
		const float SX = TimeToX(RangeStart);
		const float EX = TimeToX(RangeEnd);
		// Flags hang inward from the range edges; the end flag wins when the two overlap.
		if (P.X >= EX - 10.f && P.X <= EX + 3.f && P.Y < 14.f)
		{
			Result.Type = EHit::MarkerEnd;
		}
		else if (P.X >= SX - 3.f && P.X <= SX + 10.f && P.Y < 14.f)
		{
			Result.Type = EHit::MarkerStart;
		}
		else
		{
			Result.Type = EHit::Ruler;
		}
		return Result;
	}
	if (P.Y < TrackTop || P.Y > TrackTop + TrackHeight)
	{
		return Result;
	}
	const FAnimStitchTimeline& TL = Asset->Timeline;

	// Overlap strips first (they sit on top of the outgoing clip).
	for (int32 i = TL.Clips.Num() - 1; i >= 1; --i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		if (Clip.OverlapIn <= 0.0)
		{
			continue;
		}
		const float X0 = TimeToX(Clip.StartTime);
		const float X1 = TimeToX(Clip.StartTime + Clip.OverlapIn);
		if (P.X >= X0 && P.X <= X1)
		{
			const float H = FMath::Min(HandleWidth, (X1 - X0) / 3.f);
			Result.Index = (P.X <= X0 + H) ? i : (P.X >= X1 - H) ? i - 1 : i;
			Result.Type = (P.X <= X0 + H) ? EHit::TrimLeft : (P.X >= X1 - H) ? EHit::TrimRight : EHit::Overlap;
			return Result;
		}
	}
	for (int32 i = TL.Clips.Num() - 1; i >= 0; --i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		const float X0 = TimeToX(Clip.StartTime);
		const float X1 = TimeToX(Clip.GetEndTime());
		if (P.X >= X0 && P.X <= X1)
		{
			const float H = FMath::Min(HandleWidth, (X1 - X0) / 3.f);
			Result.Index = i;
			Result.Type = (P.X <= X0 + H) ? EHit::TrimLeft : (P.X >= X1 - H) ? EHit::TrimRight : EHit::Body;
			return Result;
		}
	}
	return Result;
}

void SAnimStitchTimeline::SelectClip(int32 Index, bool bOverlap)
{
	SelectedClip = Index;
	bOverlapSelected = bOverlap && Index > 0;
	OnSelectionChangedDelegate.ExecuteIfBound();
}

void SAnimStitchTimeline::ClampSelection()
{
	if (!Asset->Timeline.Clips.IsValidIndex(SelectedClip))
	{
		SelectClip(INDEX_NONE, false);
	}
}

// ---------------------------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------------------------

FReply SAnimStitchTimeline::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& Event)
{
	const FVector2f Local = G.AbsoluteToLocal(Event.GetScreenSpacePosition());
	if (Event.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		DragMode = EDrag::Pan;
		DragStartX = Local.X;
		InitialViewStart = ViewStart;
		bDragStarted = false;
		return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
	}
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	const FHit Hit = HitTest(Local);
	DragStartX = Local.X;
	DragIndex = Hit.Index;
	bDragStarted = false;
	DragMode = EDrag::None;

	const FAnimStitchTimeline& TL = Asset->Timeline;
	switch (Hit.Type)
	{
	case EHit::MarkerStart:
	case EHit::MarkerEnd:
		DragMode = Hit.Type == EHit::MarkerStart ? EDrag::MarkerStart : EDrag::MarkerEnd;
		break;
	case EHit::Ruler:
		DragMode = EDrag::Scrub;
		Playhead = FMath::Clamp(XToTime(Local.X), 0.0, TL.GetTotalLength());
		OnPlayheadChangedDelegate.ExecuteIfBound();
		break;
	case EHit::Body:
	case EHit::TrimLeft:
	case EHit::TrimRight:
	case EHit::Overlap:
	{
		SelectClip(Hit.Index, Hit.Type == EHit::Overlap);
		const FAnimStitchClip& Clip = TL.Clips[Hit.Index];
		InitialTrimIn = Clip.TrimIn;
		InitialTrimOut = Clip.TrimOut;
		InitialOverlap = Clip.OverlapIn;
		InitialStart = Clip.StartTime;
		DragMode = Hit.Type == EHit::Body ? EDrag::Move : Hit.Type == EHit::TrimLeft ? EDrag::TrimLeft : Hit.Type == EHit::TrimRight ? EDrag::TrimRight : EDrag::Overlap;
		break;
	}
	default:
		// Empty space deselects and also scrubs, so the playhead can be dragged from anywhere.
		SelectClip(INDEX_NONE, false);
		DragMode = EDrag::Scrub;
		Playhead = FMath::Clamp(XToTime(Local.X), 0.0, TL.GetTotalLength());
		OnPlayheadChangedDelegate.ExecuteIfBound();
		break;
	}
	return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SAnimStitchTimeline::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const FHit Hit = HitTest(G.AbsoluteToLocal(Event.GetScreenSpacePosition()));
		if (Hit.Index != INDEX_NONE && Hit.Type != EHit::Ruler)
		{
			SelectClip(Hit.Index, Hit.Type == EHit::Overlap);
		}
		OpenContextMenu(Event);
		return FReply::Handled();
	}
	const bool bEndingPan = Event.GetEffectingButton() == EKeys::MiddleMouseButton && DragMode == EDrag::Pan;
	if ((Event.GetEffectingButton() != EKeys::LeftMouseButton && !bEndingPan) || !HasMouseCapture())
	{
		return FReply::Unhandled();
	}
	DragTransaction.Reset(); // ends the undo transaction
	DragMode = EDrag::None;
	bDragStarted = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAnimStitchTimeline::OnMouseMove(const FGeometry& G, const FPointerEvent& Event)
{
	if (!HasMouseCapture() || DragMode == EDrag::None)
	{
		return FReply::Unhandled();
	}
	OnDragMove(G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X);
	return FReply::Handled();
}

void SAnimStitchTimeline::OnDragMove(float LocalX)
{
	FAnimStitchTimeline& TL = Asset->Timeline;

	if (DragMode == EDrag::Scrub)
	{
		Playhead = FMath::Clamp(XToTime(LocalX), 0.0, TL.GetTotalLength());
		OnPlayheadChangedDelegate.ExecuteIfBound();
		return;
	}
	if (DragMode == EDrag::Pan)
	{
		ViewStart = InitialViewStart - (LocalX - DragStartX) / PxPerSec;
		return;
	}
	if (!bDragStarted)
	{
		if (FMath::Abs(LocalX - DragStartX) < 3.f)
		{
			return;
		}
		bDragStarted = true;
		DragTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("EditTimelineTx", "Edit AnimStitch Timeline"));
	}
	const double Dt = (LocalX - DragStartX) / PxPerSec;
	if (DragMode == EDrag::MarkerStart || DragMode == EDrag::MarkerEnd)
	{
		double RangeStart, RangeEnd;
		Asset->GetPlaybackRange(RangeStart, RangeEnd);
		const double T = XToTime(LocalX);
		if (DragMode == EDrag::MarkerStart)
		{
			Asset->SetPlaybackRange(T, RangeEnd);
		}
		else
		{
			Asset->SetPlaybackRange(RangeStart, T);
		}
		return;
	}
	if (!TL.Clips.IsValidIndex(DragIndex))
	{
		return;
	}

	switch (DragMode)
	{
	case EDrag::TrimRight:
		Asset->SetTrim(DragIndex, InitialTrimIn, InitialTrimOut + Dt);
		break;
	case EDrag::TrimLeft:
	{
		Asset->SetTrim(DragIndex, InitialTrimIn + Dt, InitialTrimOut);
		// Keep the dragged edge under the cursor where an overlap allows it; otherwise the clip ripples.
		const double Applied = TL.Clips[DragIndex].TrimIn - InitialTrimIn;
		if (DragIndex > 0)
		{
			Asset->SetOverlap(DragIndex, InitialOverlap - Applied);
		}
		break;
	}
	case EDrag::Overlap:
		Asset->SetOverlap(DragIndex, InitialOverlap - Dt);
		break;
	case EDrag::Move:
	{
		const double DesiredStart = InitialStart + Dt;
		const double DesiredCenter = DesiredStart + TL.Clips[DragIndex].GetDuration() * 0.5;
		// One reorder step per mouse event keeps the layout from oscillating.
		if (DragIndex + 1 < TL.Clips.Num() && DesiredCenter > TL.Clips[DragIndex + 1].StartTime + TL.Clips[DragIndex + 1].GetDuration() * 0.5)
		{
			Asset->MoveClip(DragIndex, DragIndex + 1);
			++DragIndex;
		}
		else if (DragIndex > 0 && DesiredCenter < TL.Clips[DragIndex - 1].StartTime + TL.Clips[DragIndex - 1].GetDuration() * 0.5)
		{
			Asset->MoveClip(DragIndex, DragIndex - 1);
			--DragIndex;
		}
		if (DragIndex > 0)
		{
			Asset->SetOverlap(DragIndex, TL.Clips[DragIndex - 1].GetEndTime() - DesiredStart);
		}
		SelectClip(DragIndex, false);
		break;
	}
	default:
		break;
	}
}

FReply SAnimStitchTimeline::OnMouseWheel(const FGeometry& G, const FPointerEvent& Event)
{
	const float Local = G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
	if (Event.IsShiftDown())
	{
		ViewStart -= Event.GetWheelDelta() * 60.0 / PxPerSec;
	}
	else
	{
		const double AnchorTime = XToTime(Local);
		PxPerSec = FMath::Clamp(PxPerSec * FMath::Pow(1.15, Event.GetWheelDelta()), 10.0, 4000.0);
		ViewStart = AnchorTime - Local / PxPerSec;
	}
	return FReply::Handled();
}

FCursorReply SAnimStitchTimeline::OnCursorQuery(const FGeometry& G, const FPointerEvent& Event) const
{
	EHit Type = EHit::None;
	if (HasMouseCapture())
	{
		Type = DragMode == EDrag::TrimLeft ? EHit::TrimLeft : DragMode == EDrag::TrimRight ? EHit::TrimRight
			: (DragMode == EDrag::MarkerStart || DragMode == EDrag::MarkerEnd) ? EHit::MarkerStart
			: (DragMode == EDrag::Pan || DragMode == EDrag::Move) ? EHit::Body : EHit::None;
	}
	else
	{
		Type = HitTest(G.AbsoluteToLocal(Event.GetScreenSpacePosition())).Type;
	}
	switch (Type)
	{
	case EHit::TrimLeft:
	case EHit::TrimRight:
	case EHit::Overlap:
	case EHit::MarkerStart:
	case EHit::MarkerEnd:
		return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
	case EHit::Body:
		return FCursorReply::Cursor(EMouseCursor::GrabHand);
	default:
		return FCursorReply::Unhandled();
	}
}

// ---------------------------------------------------------------------------------------------
// Keyboard & commands
// ---------------------------------------------------------------------------------------------

FReply SAnimStitchTimeline::OnKeyDown(const FGeometry& G, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Delete || Event.GetKey() == EKeys::BackSpace)
	{
		DeleteSelected();
		return FReply::Handled();
	}
	if (Event.GetKey() == EKeys::D && Event.IsControlDown())
	{
		DuplicateSelected();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SAnimStitchTimeline::DeleteSelected()
{
	if (Asset->Timeline.Clips.IsValidIndex(SelectedClip))
	{
		const FScopedTransaction Tx(LOCTEXT("RemoveClipTx", "Remove Clip"));
		Asset->RemoveClip(SelectedClip);
		ClampSelection();
		OnSelectionChangedDelegate.ExecuteIfBound();
	}
}

void SAnimStitchTimeline::DuplicateSelected()
{
	if (Asset->Timeline.Clips.IsValidIndex(SelectedClip))
	{
		const FScopedTransaction Tx(LOCTEXT("DuplicateClipTx", "Duplicate Clip"));
		SelectClip(Asset->DuplicateClip(SelectedClip), false);
	}
}

int32 SAnimStitchTimeline::FindCutTarget() const
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	if (Asset->CanCutClipAt(SelectedClip, Playhead))
	{
		return SelectedClip;
	}
	for (int32 i = TL.Clips.Num() - 1; i >= 0; --i)
	{
		if (Asset->CanCutClipAt(i, Playhead))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void SAnimStitchTimeline::SplitAtPlayhead()
{
	const int32 Target = FindCutTarget();
	if (Target != INDEX_NONE)
	{
		const FScopedTransaction Tx(LOCTEXT("SplitTx", "Split Clip"));
		Asset->SplitClip(Target, Playhead);
		SelectClip(Target, false);
	}
}

void SAnimStitchTimeline::TrimStartAtPlayhead()
{
	const int32 Target = FindCutTarget();
	if (Target != INDEX_NONE)
	{
		const FScopedTransaction Tx(LOCTEXT("TrimStartTx", "Trim Clip Start"));
		Asset->TrimClipStartTo(Target, Playhead);
		SelectClip(Target, false);
	}
}

void SAnimStitchTimeline::TrimEndAtPlayhead()
{
	const int32 Target = FindCutTarget();
	if (Target != INDEX_NONE)
	{
		const FScopedTransaction Tx(LOCTEXT("TrimEndTx", "Trim Clip End"));
		Asset->TrimClipEndTo(Target, Playhead);
		SelectClip(Target, false);
	}
}

void SAnimStitchTimeline::BrowseToSelectedAsset()
{
	if (GEditor && Asset->Timeline.Clips.IsValidIndex(SelectedClip) && Asset->Timeline.Clips[SelectedClip].Sequence)
	{
		TArray<UObject*> Objects = { Asset->Timeline.Clips[SelectedClip].Sequence.Get() };
		GEditor->SyncBrowserToObjects(Objects);
	}
}

void SAnimStitchTimeline::OpenContextMenu(const FPointerEvent& Event)
{
	FMenuBuilder Menu(true, nullptr);
	const FCanExecuteAction CanCut = FCanExecuteAction::CreateSP(this, &SAnimStitchTimeline::CanEditAtPlayhead);
	Menu.AddMenuEntry(LOCTEXT("CtxSplit", "Split at Playhead"), LOCTEXT("CtxSplitTip", "Split the clip under the playhead into two clips"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchTimeline::SplitAtPlayhead), CanCut));
	Menu.AddMenuEntry(LOCTEXT("CtxTrimStart", "Trim Start to Playhead"), LOCTEXT("CtxTrimStartTip", "Cut the part of the clip before the playhead"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchTimeline::TrimStartAtPlayhead), CanCut));
	Menu.AddMenuEntry(LOCTEXT("CtxTrimEnd", "Trim End to Playhead"), LOCTEXT("CtxTrimEndTip", "Cut the part of the clip after the playhead"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchTimeline::TrimEndAtPlayhead), CanCut));
	Menu.AddSeparator();
	const FCanExecuteAction HasSelection = FCanExecuteAction::CreateLambda([this]() { return Asset->Timeline.Clips.IsValidIndex(SelectedClip); });
	Menu.AddMenuEntry(LOCTEXT("CtxBrowse", "Browse to Asset"), LOCTEXT("CtxBrowseTip", "Show the clip's animation sequence in the Content Browser"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
		FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchTimeline::BrowseToSelectedAsset), HasSelection));
	Menu.AddMenuEntry(LOCTEXT("CtxDuplicate", "Duplicate"), LOCTEXT("CtxDuplicateTip", "Duplicate the selected clip"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchTimeline::DuplicateSelected), HasSelection));
	Menu.AddMenuEntry(LOCTEXT("CtxRemove", "Remove"), LOCTEXT("CtxRemoveTip", "Remove the selected clip"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchTimeline::DeleteSelected), HasSelection));

	const FWidgetPath Path = Event.GetEventPath() != nullptr ? *Event.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), Path, Menu.MakeWidget(), Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

void SAnimStitchTimeline::SetSelectedBlendCurve(EAnimStitchBlendCurve Curve)
{
	if (bOverlapSelected && Asset->Timeline.Clips.IsValidIndex(SelectedClip))
	{
		const FScopedTransaction Tx(LOCTEXT("SetCurveTx", "Set Crossfade Curve"));
		Asset->SetBlendCurve(SelectedClip, Curve);
	}
}

void SAnimStitchTimeline::ZoomToFit()
{
	const double Length = FMath::Max(Asset->Timeline.GetTotalLength(), 0.5);
	const double Width = FMath::Max(GetTickSpaceGeometry().GetLocalSize().X, 100.f);
	PxPerSec = (Width - 40.0) / Length;
	ViewStart = -20.0 / PxPerSec;
}

// ---------------------------------------------------------------------------------------------
// Drag & drop from the Content Browser
// ---------------------------------------------------------------------------------------------

bool SAnimStitchTimeline::GetDroppedSequences(const FDragDropEvent& Event, TArray<UAnimSequence*>& Out, bool bLoad) const
{
	const TSharedPtr<FAssetDragDropOp> Op = Event.GetOperationAs<FAssetDragDropOp>();
	if (!Op.IsValid())
	{
		return false;
	}
	for (const FAssetData& Data : Op->GetAssets())
	{
		if (Data.GetClass() && Data.GetClass()->IsChildOf(UAnimSequence::StaticClass()))
		{
			Out.Add(bLoad ? Cast<UAnimSequence>(Data.GetAsset()) : nullptr);
		}
	}
	return Out.Num() > 0;
}

int32 SAnimStitchTimeline::TimeToInsertIndex(double Time) const
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	for (int32 i = 0; i < TL.Clips.Num(); ++i)
	{
		if (Time < TL.Clips[i].StartTime + TL.Clips[i].GetDuration() * 0.5)
		{
			return i;
		}
	}
	return TL.Clips.Num();
}

void SAnimStitchTimeline::OnDragEnter(const FGeometry& G, const FDragDropEvent& Event)
{
	TArray<UAnimSequence*> Probe;
	bDropHover = GetDroppedSequences(Event, Probe, false);
}

void SAnimStitchTimeline::OnDragLeave(const FDragDropEvent& Event)
{
	bDropHover = false;
}

FReply SAnimStitchTimeline::OnDragOver(const FGeometry& G, const FDragDropEvent& Event)
{
	TArray<UAnimSequence*> Probe;
	if (!GetDroppedSequences(Event, Probe, false))
	{
		return FReply::Unhandled();
	}
	const FAnimStitchTimeline& TL = Asset->Timeline;
	const int32 Index = TimeToInsertIndex(XToTime(G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X));
	bDropHover = true;
	DropTime = Index < TL.Clips.Num() ? TL.Clips[Index].StartTime : TL.GetTotalLength();
	return FReply::Handled();
}

FReply SAnimStitchTimeline::OnDrop(const FGeometry& G, const FDragDropEvent& Event)
{
	bDropHover = false;
	TArray<UAnimSequence*> Sequences;
	if (!GetDroppedSequences(Event, Sequences, true))
	{
		return FReply::Unhandled();
	}
	const int32 Index = TimeToInsertIndex(XToTime(G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X));
	TArray<FText> Warnings;
	{
		FScopedTransaction Tx(LOCTEXT("AddClipsTx", "Add Clips"));
		const int32 Added = Asset->AddSequences(Sequences, Index, Warnings);
		if (Added == 0)
		{
			Tx.Cancel();
		}
		else
		{
			SelectClip(Index, false);
		}
	}
	if (Warnings.Num() > 0)
	{
		OnWarningsDelegate.ExecuteIfBound(Warnings);
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE



