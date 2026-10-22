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
#include "AnimStitchManualEdits.h"
#include "AnimStitchShortcuts.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace AnimStitchTimelineUI
{
	const FLinearColor Background(0.04f, 0.04f, 0.045f);
	const FLinearColor RulerBg(0.08f, 0.08f, 0.09f);
	const FLinearColor ClipColors[2] = { FLinearColor(0.16f, 0.34f, 0.55f), FLinearColor(0.20f, 0.46f, 0.34f) };
	const FLinearColor AuthorityColor(0.50f, 0.24f, 0.60f);
const FLinearColor OverlapColor(0.95f, 0.65f, 0.15f, 0.55f);
	/** Gap crossfade span: same look as the overlap, different colour (teal vs amber). */
	const FLinearColor GapColor(0.1f, 0.65f, 0.75f, 0.5f);
	const FLinearColor GapLineColor(0.7f, 1.f, 1.f, 1.f);
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
	SetToolTipText(TAttribute<FText>::CreateSP(this, &SAnimStitchTimeline::GetHoverTipText));
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
	const double DisplayFps = GetDisplayFps();
	static const double Steps[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0 };
	static const double FrameSteps[] = { 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 1800, 3600 };
	double Step = bShowFrames ? FrameSteps[UE_ARRAY_COUNT(FrameSteps) - 1] / DisplayFps : Steps[UE_ARRAY_COUNT(Steps) - 1];
	if (bShowFrames)
	{
		for (double S : FrameSteps) { if (S / DisplayFps * PxPerSec >= 60.0) { Step = S / DisplayFps; break; } }
	}
	else
	{
		for (double S : Steps) { if (S * PxPerSec >= 70.0) { Step = S; break; } }
	}
	for (double T = FMath::CeilToDouble(XToTime(LabelWidth) / Step) * Step; TimeToX(T) < Size.X; T += Step)
	{
		const float X = TimeToX(T);
		DrawLine(Out, Layer + 2, G, { FVector2f(X, RulerHeight - 8.f), FVector2f(X, RulerHeight) }, FLinearColor(1, 1, 1, 0.6f));
		const FString TickLabel = bShowFrames ? FString::Printf(TEXT("%04d"), FMath::RoundToInt(T * DisplayFps)) : (Step >= 1.0 ? FString::Printf(TEXT("%.0fs"), T) : FString::Printf(TEXT("%.2fs"), T));
		PaintLabel(Out, Layer + 2, G, FVector2f(X + 3.f, 4.f), TickLabel, FLinearColor(1, 1, 1, 0.65f));
		if (T > 1e6) { break; }
	}

	// Foot plant lanes (foot lock): amber = detected plant, green = manually pinned, red line = detected but unpinned by hand.
	if (IsFootLaneVisible())
	{
		const bool bHavePlants = Plants.IsValid() && Plants->bValid;
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			const float LaneY = GetLaneTop(Foot);
			DrawRect(Out, Layer + 2, G, FVector2f(0.f, LaneY), FVector2f(Size.X, LaneHeight), FLinearColor(0.f, 0.f, 0.f, 0.35f));
			PaintLabel(Out, Layer + 3, G, FVector2f(3.f, LaneY - 2.f), Foot == 0 ? TEXT("L") : TEXT("R"), FLinearColor(1, 1, 1, 0.6f));
			if (!bHavePlants)
			{
				if (Foot == 0 && Plants.IsValid())
				{
					PaintLabel(Out, Layer + 3, G, FVector2f(18.f, LaneY - 2.f), Plants->Problem.ToString(), FLinearColor(1.f, 0.5f, 0.4f));
				}
				continue;
			}
			const TArray<uint8>& Mask = Plants->Mask[Foot];
			for (int32 f = 0; f < Mask.Num();)
			{
				if (Mask[f] == AnimStitchPlant::None) { ++f; continue; }
				int32 End = f;
				while (End + 1 < Mask.Num() && Mask[End + 1] == Mask[f]) { ++End; }
				const float X0 = TimeToX((f - 0.5) / Plants->Fps);
				const float X1 = TimeToX((End + 0.5) / Plants->Fps);
				if (Mask[f] == AnimStitchPlant::Suppressed)
				{
					DrawRect(Out, Layer + 3, G, FVector2f(X0, LaneY + LaneHeight - 2.f), FVector2f(FMath::Max(1.f, X1 - X0), 2.f), FLinearColor(0.9f, 0.2f, 0.2f));
				}
				else
				{
					DrawRect(Out, Layer + 3, G, FVector2f(X0, LaneY + 1.f), FVector2f(FMath::Max(1.f, X1 - X0), LaneHeight - 2.f),
						Mask[f] == AnimStitchPlant::ManualPin ? FLinearColor(0.3f, 0.85f, 0.4f) : FLinearColor(0.95f, 0.65f, 0.15f));
				}
				f = End + 1;
			}
		}
		if (bHavePlants)
		{
			for (int32 Foot = 0; Foot < 2; ++Foot)
			{
				for (const FAnimStitchFootLockResult::FEventMark& Mark : Plants->Events[Foot])
				{
					const float X = TimeToX(Mark.Time);
					const FLinearColor Tick = Mark.Action == EAnimStitchFootAction::Lock ? FLinearColor(0.4f, 1.f, 0.5f) : Mark.Action == EAnimStitchFootAction::Unlock ? FLinearColor(1.f, 0.35f, 0.35f) : FLinearColor(0.8f, 0.8f, 0.8f);
					DrawRect(Out, Layer + 5, G, FVector2f(X - 1.f, GetLaneTop(Foot) - 1.f), FVector2f(2.f, LaneHeight + 2.f), Tick);
				}
				// Drag handles on both ends of a manually locked region (start = Lock mark, end = the next mark).
				const TArray<FAnimStitchFootLockResult::FEventMark>& Marks = Plants->Events[Foot];
				for (int32 m = 0; m < Marks.Num(); ++m)
				{
					if (Marks[m].Action != EAnimStitchFootAction::Lock) { continue; }
					for (const int32 Index : { m, m + 1 })
					{
						if (!Marks.IsValidIndex(Index)) { continue; }
						DrawRect(Out, Layer + 6, G, FVector2f(TimeToX(Marks[Index].Time) - 3.f, GetLaneTop(Foot) - 2.f), FVector2f(6.f, LaneHeight + 4.f), FLinearColor(1.f, 1.f, 1.f, 0.85f));
					}
				}
			}
		}
		if (DragMode == EDrag::FootLane && bDragStarted)
		{
			const float X0 = TimeToX(FMath::Min(FootDragStartTime, FootDragCurTime));
			const float X1 = TimeToX(FMath::Max(FootDragStartTime, FootDragCurTime));
			DrawRect(Out, Layer + 4, G, FVector2f(X0, GetLaneTop(DragFoot)), FVector2f(FMath::Max(1.f, X1 - X0), LaneHeight), FLinearColor(0.3f, 0.85f, 0.4f, 0.6f));
		}
	}

	// Clips
	for (int32 i = 0; i < TL.Clips.Num(); ++i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		const float X0 = TimeToX(Clip.StartTime);
		const float X1 = TimeToX(Clip.GetEndTime());
		const FVector2f Pos(X0, GetTrackTop());
		const FVector2f BoxSize(FMath::Max(1.f, X1 - X0), TrackHeight);

		const bool bAuthority = i == TL.RootAuthorityIndex;
		FLinearColor Color = bAuthority ? AuthorityColor : ClipColors[i & 1];
		DrawRect(Out, Layer + 3, G, Pos, BoxSize, Color);
		DrawRect(Out, Layer + 4, G, Pos, FVector2f(FMath::Min(HandleWidth, BoxSize.X * 0.5f), TrackHeight), Handle);
		DrawRect(Out, Layer + 4, G, FVector2f(X1 - FMath::Min(HandleWidth, BoxSize.X * 0.5f), GetTrackTop()), FVector2f(FMath::Min(HandleWidth, BoxSize.X * 0.5f), TrackHeight), Handle);

		const FGeometry Child = G.MakeChild(BoxSize, FSlateLayoutTransform(Pos));
		Out.PushClip(FSlateClippingZone(Child));
		PaintLabel(Out, Layer + 6, G, Pos + FVector2f(HandleWidth + 3.f, 3.f), Clip.Sequence ? Clip.Sequence->GetName() : FString(TEXT("<missing>")), FLinearColor::White);
PaintLabel(Out, Layer + 6, G, Pos + FVector2f(HandleWidth + 3.f, 17.f), FString::Printf(TEXT("%.2fs"), Clip.GetDuration()), FLinearColor(1, 1, 1, 0.75f));
		if (!FMath::IsNearlyEqual(Clip.PlayRate, 1.0, 1e-4))
		{
			// Small rate tag, top right of the clip: shown whenever the clip does not play at 1x.
			PaintLabel(Out, Layer + 6, G, Pos + FVector2f(FMath::Max(HandleWidth + 3.f, BoxSize.X - 52.f), 3.f), FString::Printf(TEXT("%.2f\u00d7"), Clip.PlayRate), FLinearColor(1.f, 0.85f, 0.4f));
		}
		if (bAuthority)
		{
			PaintLabel(Out, Layer + 6, G, Pos + FVector2f(HandleWidth + 3.f, 31.f), LOCTEXT("AuthorityLabel", "Root Motion Authority").ToString(), FLinearColor(1.f, 0.9f, 0.4f));
		}
		if (Clip.bBatchSlot)
		{
			PaintLabel(Out, Layer + 6, G, Pos + FVector2f(HandleWidth + 3.f, 45.f), LOCTEXT("BatchLabel", "Batch Slot").ToString(), FLinearColor(0.6f, 0.9f, 1.f));
		}
		Out.PopClip();

		if (IsClipSelected(i) && !bOverlapSelected)
		{
			DrawLine(Out, Layer + 8, G, { Pos, Pos + FVector2f(BoxSize.X, 0), Pos + BoxSize, Pos + FVector2f(0, BoxSize.Y), Pos }, Selected, 2.f);
		}
	}

	// Overlaps (crossfades)
	for (int32 i = 1; i < TL.Clips.Num(); ++i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		if (FMath::IsNearlyZero(Clip.OverlapIn))
		{
			continue;
		}
		// Overlap = span at the start of this clip; gap = span between the previous clip's end and this clip's start (distinct colour).
		const bool bGap = Clip.OverlapIn < 0.0;
		const float X0 = TimeToX(bGap ? TL.Clips[i - 1].GetEndTime() : Clip.StartTime);
		const float X1 = TimeToX(bGap ? Clip.StartTime : Clip.StartTime + Clip.OverlapIn);
		const float W = FMath::Max(1.f, X1 - X0);
		DrawRect(Out, Layer + 9, G, FVector2f(X0, GetTrackTop()), FVector2f(W, TrackHeight), bGap ? GapColor : OverlapColor);

		// Weight curves: outgoing falls, incoming rises.
		TArray<FVector2f> Fall, Rise;
		constexpr int32 N = 16;
		for (int32 k = 0; k <= N; ++k)
		{
			const float A = float(k) / N;
			const float Wt = Clip.BlendIn == EAnimStitchBlendCurve::Smoothstep ? A * A * (3.f - 2.f * A) : A;
			Rise.Add(FVector2f(X0 + W * A, GetTrackTop() + TrackHeight - 4.f - (TrackHeight - 8.f) * Wt));
			Fall.Add(FVector2f(X0 + W * A, GetTrackTop() + 4.f + (TrackHeight - 8.f) * Wt));
		}
		const FLinearColor LineColor = bGap ? GapLineColor : FLinearColor::White;
		DrawLine(Out, Layer + 10, G, Rise, LineColor, 1.5f);
		DrawLine(Out, Layer + 10, G, Fall, bGap ? GapLineColor.CopyWithNewOpacity(0.6f) : FLinearColor(1, 1, 1, 0.6f), 1.5f);
		if (W > 36.f)
		{
			PaintLabel(Out, Layer + 11, G, FVector2f(X0 + 3.f, GetTrackTop() + TrackHeight - 16.f), FString::Printf(TEXT("%.2fs"), FMath::Abs(Clip.OverlapIn)), LineColor);
		}
		if (i == SelectedClip && bOverlapSelected)
		{
			const FVector2f P(X0, GetTrackTop()), S(W, TrackHeight);
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
		DrawRect(Out, Layer + 14, G, FVector2f(SX, 0.f), FVector2f(8.f, 10.f), StartColor);
		DrawRect(Out, Layer + 14, G, FVector2f(EX - 8.f, 0.f), FVector2f(8.f, 10.f), EndColor);
	}

	PaintTrackLanes(Out, Layer + 7, G);

	// Drop insertion marker
	if (bDropHover)
	{
		const float X = TimeToX(DropTime);
		DrawLine(Out, Layer + 13, G, { FVector2f(X, RulerHeight), FVector2f(X, Size.Y) }, FLinearColor(0.3f, 1.f, 0.4f), 2.f);
	}
	if (TL.Clips.Num() == 0)
	{
		PaintLabel(Out, Layer + 13, G, FVector2f(14.f, GetTrackTop() + 22.f), LOCTEXT("EmptyHint", "Drag animation sequences from the Content Browser here").ToString(), FLinearColor(1, 1, 1, 0.5f));
	}

	// Playhead
	const float PX = TimeToX(Playhead);
	DrawLine(Out, Layer + 14, G, { FVector2f(PX, 0.f), FVector2f(PX, Size.Y) }, FLinearColor(1.f, 0.25f, 0.25f), 1.5f);

	// Grab handle on the lower ruler (the flags above own the top strip).
	DrawRect(Out, Layer + 15, G, FVector2f(PX - 5.f, RulerHeight - 12.f), FVector2f(10.f, 12.f), FLinearColor(1.f, 0.25f, 0.25f));

	if (DragMode == EDrag::Marquee && bDragStarted)
	{
		const FVector2f Min(FMath::Min(MarqueeStart.X, MarqueeCurrent.X), FMath::Min(MarqueeStart.Y, MarqueeCurrent.Y));
		const FVector2f Max(FMath::Max(MarqueeStart.X, MarqueeCurrent.X), FMath::Max(MarqueeStart.Y, MarqueeCurrent.Y));
		DrawRect(Out, Layer + 15, G, Min, Max - Min, FLinearColor(0.2f, 0.5f, 1.f, 0.18f));
		DrawLine(Out, Layer + 15, G, { Min, FVector2f(Max.X, Min.Y), Max, FVector2f(Min.X, Max.Y), Min }, FLinearColor(0.35f, 0.65f, 1.f, 0.9f), 1.f);
	}

	PaintLeftColumn(Out, Layer + 16, G);

	Out.PopClip();
	return Layer + 24;
}

// ---------------------------------------------------------------------------------------------
// Hit testing & selection
// ---------------------------------------------------------------------------------------------

SAnimStitchTimeline::FHit SAnimStitchTimeline::HitTest(const FVector2f& P) const
{
	FHit Result;
	if (FMath::Abs(P.X - LabelWidth) <= 3.f)
	{
		Result.Type = EHit::Splitter;
		return Result;
	}
	if (P.X < LabelWidth)
	{
		// Track column: only the manual-correction rows react.
		for (int32 i = 0; i < Asset->Timeline.Tracks.Num(); ++i)
		{
			const float Top = GetTrackLaneTop(i);
			if (P.Y >= GetTracksTop() && P.Y >= Top && P.Y <= Top + TrackLaneHeight)
			{
				Result.Type = EHit::TrackLane;
				Result.Index = i;
				GetLaneButton(P.X, P.Y - Top, i, Result.Type);
				return Result;
			}
		}
		return Result;
	}
	if (P.Y < RulerHeight)
	{
		double RangeStart, RangeEnd;
		Asset->GetPlaybackRange(RangeStart, RangeEnd);
		const float SX = TimeToX(RangeStart);
		const float EX = TimeToX(RangeEnd);
		// Flags hang inward from the range edges; the end flag wins when the two overlap.
		if (P.X >= EX - 8.f && P.X <= EX + 2.f && P.Y < 10.f)
		{
			Result.Type = EHit::MarkerEnd;
		}
		else if (P.X >= SX - 2.f && P.X <= SX + 8.f && P.Y < 10.f)
		{
			Result.Type = EHit::MarkerStart;
		}
		else
		{
			Result.Type = EHit::Ruler;
		}
		return Result;
	}
	if (IsFootLaneVisible())
	{
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			if (P.Y >= GetLaneTop(Foot) - 2.f && P.Y <= GetLaneTop(Foot) + LaneHeight + 2.f)
			{
				if (Plants.IsValid() && Plants->bValid)
				{
					// A handle on either end of a manual lock wins over painting a new region.
					const TArray<FAnimStitchFootLockResult::FEventMark>& Marks = Plants->Events[Foot];
					float BestDistance = 6.f;
					for (int32 m = 0; m < Marks.Num(); ++m)
					{
						if (Marks[m].Action != EAnimStitchFootAction::Lock) { continue; }
						for (const int32 Index : { m, m + 1 })
						{
							if (!Marks.IsValidIndex(Index)) { continue; }
							const float Distance = FMath::Abs(P.X - TimeToX(Marks[Index].Time));
							if (Distance <= BestDistance) { BestDistance = Distance; Result.Type = EHit::FootHandle; Result.Index = Foot; Result.Sub = Index; }
						}
					}
					if (Result.Type == EHit::FootHandle) { return Result; }
					// Inside a manual lock (between its Lock mark and the next mark): drag moves the whole region.
					const double Time = XToTime(P.X);
					for (int32 m = 0; m < Marks.Num(); ++m)
					{
						if (Marks[m].Action != EAnimStitchFootAction::Lock) { continue; }
						const double End = Marks.IsValidIndex(m + 1) ? Marks[m + 1].Time : Asset->Timeline.GetTotalLength();
						if (Time >= Marks[m].Time && Time <= End) { Result.Type = EHit::FootRegion; Result.Index = Foot; Result.Sub = m; return Result; }
					}
				}
				Result.Type = EHit::FootLane;
				Result.Index = Foot;
				return Result;
			}
		}
	}
	// Manual correction lanes below the clip track.
	for (int32 i = 0; i < Asset->Timeline.Tracks.Num(); ++i)
	{
		const float Top = GetTrackLaneTop(i);
		if (P.Y >= GetTracksTop() && P.Y >= Top && P.Y <= Top + TrackLaneHeight)
		{
			Result.Type = EHit::TrackLane;
			Result.Index = i;
			const FAnimStitchTrack& Track = Asset->Timeline.Tracks[i];
			for (int32 k = 0; k < Track.Keys.Num(); ++k)
			{
				if (FMath::Abs(P.X - TimeToX(Track.Keys[k].Time)) <= 9.f)
				{
					Result.Type = EHit::TrackKey;
					Result.Sub = k;
					break;
				}
			}
			return Result;
		}
	}
	// The Translation / Rotation / Scale sub-lanes under an expanded bone track: each holds only its own channel's keys.
	for (int32 i = 0; i < Asset->Timeline.Tracks.Num(); ++i)
	{
		if (!HasChannelLanes(i)) { continue; }
		const float RowTop = GetChannelRowTop(i, 0);
		if (P.Y < GetTracksTop() || P.Y < RowTop || P.Y >= RowTop + 3.f * GetChannelRowHeight()) { continue; }
		const int32 Row = FMath::Clamp(int32((P.Y - RowTop) / GetChannelRowHeight()), 0, 2);
		const uint8 Bit = ChannelBitForRow(Row);
		Result.Type = EHit::TrackLane;
		Result.Index = i;
		Result.Channel = Bit;
		const FAnimStitchTrack& Track = Asset->Timeline.Tracks[i];
		for (int32 k = 0; k < Track.Keys.Num(); ++k)
		{
			if ((Track.Keys[k].Channels & Bit) && FMath::Abs(P.X - TimeToX(Track.Keys[k].Time)) <= 8.f)
			{
				Result.Type = EHit::TrackKey;
				Result.Sub = k;
				break;
			}
		}
		return Result;
	}
	if (P.Y < GetTrackTop() || P.Y > GetTrackTop() + TrackHeight)
	{
		return Result;
	}
	const FAnimStitchTimeline& TL = Asset->Timeline;

	// Overlap strips first (they sit on top of the outgoing clip).
	for (int32 i = TL.Clips.Num() - 1; i >= 1; --i)
	{
		const FAnimStitchClip& Clip = TL.Clips[i];
		if (Clip.OverlapIn < 0.0)
		{
			// Gap crossfade: the whole span is the grab area (drag = change the spacing).
			const float GX0 = TimeToX(TL.Clips[i - 1].GetEndTime());
			const float GX1 = TimeToX(Clip.StartTime);
			if (P.X >= GX0 && P.X <= GX1)
			{
				Result.Index = i;
				Result.Type = EHit::Overlap;
				return Result;
			}
			continue;
		}
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

void SAnimStitchTimeline::UpdateMarquee(const FVector2f& Local)
{
	MarqueeCurrent = Local;
	bDragStarted = bDragStarted || (Local - MarqueeStart).Size() >= 3.f;
	if (!bDragStarted) { return; }
	const float X0 = FMath::Min(MarqueeStart.X, Local.X), X1 = FMath::Max(MarqueeStart.X, Local.X);
	const float Y0 = FMath::Min(MarqueeStart.Y, Local.Y), Y1 = FMath::Max(MarqueeStart.Y, Local.Y);
	SelectedKeys = MarqueeInitial;
	const FAnimStitchTimeline& TL = Asset->Timeline;
	FKeyRef Last;
	for (int32 i = 0; i < TL.Tracks.Num(); ++i)
	{
		const float CenterY = GetTrackLaneTop(i) + TrackLaneHeight * 0.5f;
		if (HasChannelLanes(i))
		{
			for (int32 Row = 0; Row < 3; ++Row)
			{
				const float RowY = GetChannelRowTop(i, Row) + GetChannelRowHeight() * 0.5f;
				if (RowY < Y0 || RowY > Y1) { continue; }
				const uint8 Bit = ChannelBitForRow(Row);
				for (int32 k = 0; k < TL.Tracks[i].Keys.Num(); ++k)
				{
					const float X = TimeToX(TL.Tracks[i].Keys[k].Time);
					if ((TL.Tracks[i].Keys[k].Channels & Bit) && X >= LabelWidth && X >= X0 && X <= X1) { SelectedKeys.AddUnique(FKeyRef{ i, k, Bit }); Last = FKeyRef{ i, k, Bit }; }
				}
			}
		}
		if (CenterY < Y0 || CenterY > Y1) { continue; }
		for (int32 k = 0; k < TL.Tracks[i].Keys.Num(); ++k)
		{
			const float X = TimeToX(TL.Tracks[i].Keys[k].Time);
			if (X >= LabelWidth && X >= X0 && X <= X1) { SelectedKeys.AddUnique(FKeyRef{ i, k }); Last = FKeyRef{ i, k }; }
		}
	}
	// The primary selection (what the value panel shows) follows a single hit.
	if (SelectedKeys.Num() == 1) { SelectedTrack = SelectedKeys[0].Track; SelectedKey = SelectedKeys[0].Key; SelectedChannel = SelectedKeys[0].Channel; }
	else { SelectedKey = INDEX_NONE; SelectedChannel = 0; if (Last.Track != INDEX_NONE) { SelectedTrack = Last.Track; } }
}

namespace
{
	struct FClipboardEntry
	{
		FName Bone;
		EAnimStitchTrackKind Kind = EAnimStitchTrackKind::Bone;
		TArray<FAnimStitchTrackKey> Keys; // Time is relative to the earliest copied key
	};
	TArray<FClipboardEntry> KeyClipboard;
}

bool SAnimStitchTimeline::HasKeyClipboard()
{
	return KeyClipboard.Num() > 0;
}

bool SAnimStitchTimeline::CanPasteKeys() const
{
	for (const FClipboardEntry& Entry : KeyClipboard)
	{
		if (Entry.Keys.Num() > 0 && HasTrackFor(Entry.Bone, Entry.Kind)) { return true; }
	}
	return false;
}

bool SAnimStitchTimeline::CanDuplicateKeysToPlayhead() const
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	double Earliest = TNumericLimits<double>::Max();
	for (const FKeyRef& Ref : SelectedKeys)
	{
		if (TL.Tracks.IsValidIndex(Ref.Track) && TL.Tracks[Ref.Track].Keys.IsValidIndex(Ref.Key)) { Earliest = FMath::Min(Earliest, TL.Tracks[Ref.Track].Keys[Ref.Key].Time); }
	}
	if (TL.Tracks.IsValidIndex(SelectedTrack) && TL.Tracks[SelectedTrack].Keys.IsValidIndex(SelectedKey)) { Earliest = FMath::Min(Earliest, TL.Tracks[SelectedTrack].Keys[SelectedKey].Time); }
	return Earliest != TNumericLimits<double>::Max() && FMath::Abs(Earliest - Playhead) > 1.0 / 120.0;
}

bool SAnimStitchTimeline::SelectClipForSequence(const FSoftObjectPath& Path)
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	int32 First = INDEX_NONE, AfterCurrent = INDEX_NONE;
	for (int32 i = 0; i < TL.Clips.Num(); ++i)
	{
		if (!TL.Clips[i].Sequence || FSoftObjectPath(TL.Clips[i].Sequence) != Path) { continue; }
		if (First == INDEX_NONE) { First = i; }
		if (AfterCurrent == INDEX_NONE && i > SelectedClip) { AfterCurrent = i; }
	}
	if (First == INDEX_NONE) { return false; }
	const bool bCurrentUses = TL.Clips.IsValidIndex(SelectedClip) && TL.Clips[SelectedClip].Sequence && FSoftObjectPath(TL.Clips[SelectedClip].Sequence) == Path;
	SelectClip(bCurrentUses && AfterCurrent != INDEX_NONE ? AfterCurrent : First, false);
	return true;
}

void SAnimStitchTimeline::SelectTrackFromViewport(int32 Track)
{
	if (!Asset->Timeline.Tracks.IsValidIndex(Track) || Track == SelectedTrack) { return; }
	SelectedTrack = Track;
	SelectedKey = INDEX_NONE;
	SelectedKeys.Reset();
	// Scroll the lane into view.
	const float Top = Track * (TrackLaneHeight + 2.f) + ExpandedAbove(Track) * ExpanderHeight;
	const float View = GetTrackViewHeight();
	if (Top < VScroll) { SetTrackScroll(Top); }
	else if (Top + TrackLaneHeight > VScroll + View) { SetTrackScroll(Top + TrackLaneHeight - View + 4.f); }
}

void SAnimStitchTimeline::CopySelectedKeys()
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	TArray<FKeyRef> Refs = SelectedKeys;
	if (Refs.Num() == 0 && TL.Tracks.IsValidIndex(SelectedTrack) && TL.Tracks[SelectedTrack].Keys.IsValidIndex(SelectedKey)) { Refs.Add(FKeyRef{ SelectedTrack, SelectedKey }); }
	double Earliest = TNumericLimits<double>::Max();
	for (const FKeyRef& Ref : Refs)
	{
		if (TL.Tracks.IsValidIndex(Ref.Track) && TL.Tracks[Ref.Track].Keys.IsValidIndex(Ref.Key)) { Earliest = FMath::Min(Earliest, TL.Tracks[Ref.Track].Keys[Ref.Key].Time); }
	}
	if (Earliest == TNumericLimits<double>::Max()) { return; }
	KeyClipboard.Reset();
	for (const FKeyRef& Ref : Refs)
	{
		if (!TL.Tracks.IsValidIndex(Ref.Track) || !TL.Tracks[Ref.Track].Keys.IsValidIndex(Ref.Key)) { continue; }
		const FAnimStitchTrack& Track = TL.Tracks[Ref.Track];
		FClipboardEntry* Entry = KeyClipboard.FindByPredicate([&Track](const FClipboardEntry& E) { return E.Bone == Track.Bone && E.Kind == Track.Kind; });
		if (!Entry) { Entry = &KeyClipboard.AddDefaulted_GetRef(); Entry->Bone = Track.Bone; Entry->Kind = Track.Kind; }
		FAnimStitchTrackKey Key = Track.Keys[Ref.Key];
		Key.Time -= Earliest;
		if (Ref.Channel != 0) { Key.Channels = Ref.Channel; } // a key copied from a channel lane carries just that channel
		Entry->Keys.Add(Key);
	}
}

void SAnimStitchTimeline::PasteKeys()
{
	if (KeyClipboard.Num() == 0) { return; }
	FAnimStitchTimeline& TL = Asset->Timeline;
	const FScopedTransaction Tx(LOCTEXT("PasteKeysTx", "Paste Keys"));
	for (const FClipboardEntry& Entry : KeyClipboard)
	{
		// The same bone track: the selected track if it matches, else the first track for that bone.
		int32 TrackIndex = INDEX_NONE;
		if (TL.Tracks.IsValidIndex(SelectedTrack) && TL.Tracks[SelectedTrack].Bone == Entry.Bone && TL.Tracks[SelectedTrack].Kind == Entry.Kind) { TrackIndex = SelectedTrack; }
		for (int32 i = 0; i < TL.Tracks.Num() && TrackIndex == INDEX_NONE; ++i)
		{
			if (TL.Tracks[i].Bone == Entry.Bone && TL.Tracks[i].Kind == Entry.Kind) { TrackIndex = i; }
		}
		if (TrackIndex == INDEX_NONE) { continue; }
		for (const FAnimStitchTrackKey& Source : Entry.Keys)
		{
			FAnimStitchTrackKey Key = Source;
			Key.Time = FMath::Max(0.0, Playhead + Source.Time);
			Asset->SetTrackChannelKey(TrackIndex, Key, Key.Channels); // only the channels the copied key held; locked channels are skipped
		}
	}
	// Select what was pasted.
	SelectedKeys.Reset();
	for (const FClipboardEntry& Entry : KeyClipboard)
	{
		for (int32 i = 0; i < TL.Tracks.Num(); ++i)
		{
			if (TL.Tracks[i].Bone != Entry.Bone || TL.Tracks[i].Kind != Entry.Kind) { continue; }
			for (const FAnimStitchTrackKey& Source : Entry.Keys)
			{
				const int32 Index = FAnimStitchManualEdits::FindKey(TL.Tracks[i], FMath::Max(0.0, Playhead + Source.Time));
				if (Index != INDEX_NONE) { SelectedKeys.AddUnique(FKeyRef{ i, Index }); }
			}
			break;
		}
	}
	SelectedKey = INDEX_NONE;
}

void SAnimStitchTimeline::DeleteSelectedKeys()
{
	TArray<FKeyRef> Refs = SelectedKeys;
	Refs.Sort([](const FKeyRef& A, const FKeyRef& B) { return A.Track != B.Track ? A.Track < B.Track : A.Key > B.Key; }); // highest key first so indices stay valid
	const FScopedTransaction Tx(LOCTEXT("DeleteKeysTx", "Delete Keys"));
	// One removal per key with all its selected channels merged, so a key that loses its last channel is removed exactly once.
	for (int32 i = 0; i < Refs.Num();)
	{
		uint8 Mask = 0;
		int32 j = i;
		for (; j < Refs.Num() && Refs[j].Track == Refs[i].Track && Refs[j].Key == Refs[i].Key; ++j) { Mask |= Refs[j].Channel ? Refs[j].Channel : AnimStitchChannel::All; }
		Asset->RemoveTrackChannelKey(Refs[i].Track, Refs[i].Key, Mask);
		i = j;
	}
	SelectedKeys.Reset();
	SelectedKey = INDEX_NONE;
	SelectedChannel = 0;
}

void SAnimStitchTimeline::DeselectAll()
{
	SelectedTrack = INDEX_NONE;
	SelectedKey = INDEX_NONE;
	SelectedChannel = 0;
	SelectedKeys.Reset();
	++TrackSelectionSerial; // the panel also drops the viewport control / bone the track had selected, so Details empties
	SelectClip(INDEX_NONE, false);
}

TArray<int32> SAnimStitchTimeline::GetSelectedClipIndices() const
{
	TArray<int32> Result;
	if (!Asset) { return Result; }
	for (int32 i = 0; i < Asset->Timeline.Clips.Num(); ++i)
	{
		if (IsClipSelected(i)) { Result.Add(i); }
	}
	return Result;
}

void SAnimStitchTimeline::ToggleClipInSelection(int32 Index)
{
	if (!Asset->Timeline.Clips.IsValidIndex(Index)) { return; }
	Asset->EnsureClipIds();
	const FGuid Id = Asset->Timeline.Clips[Index].ClipId;
	// The previously selected primary clip joins the id set, so everything shown as selected is in it.
	if (Asset->Timeline.Clips.IsValidIndex(SelectedClip)) { SelectedClipIds.AddUnique(Asset->Timeline.Clips[SelectedClip].ClipId); }
	if (SelectedClipIds.Contains(Id))
	{
		SelectedClipIds.Remove(Id);
		if (SelectedClip == Index)
		{
			SelectedClip = INDEX_NONE;
			for (int32 i = 0; i < Asset->Timeline.Clips.Num(); ++i)
			{
				if (SelectedClipIds.Contains(Asset->Timeline.Clips[i].ClipId)) { SelectedClip = i; }
			}
		}
	}
	else
	{
		SelectedClipIds.AddUnique(Id);
		SelectedClip = Index;
		SelectedTrack = INDEX_NONE;
		SelectedKey = INDEX_NONE;
		SelectedChannel = 0;
		SelectedKeys.Reset();
	}
	bOverlapSelected = false;
	OnSelectionChangedDelegate.ExecuteIfBound();
}

void SAnimStitchTimeline::SelectClip(int32 Index, bool bOverlap)
{
	SelectedClip = Index;
	SelectedClipIds.Reset();
	if (Asset && Asset->Timeline.Clips.IsValidIndex(Index) && Asset->Timeline.Clips[Index].ClipId.IsValid()) { SelectedClipIds.Add(Asset->Timeline.Clips[Index].ClipId); }
	if (Index != INDEX_NONE)
	{
		SelectedTrack = INDEX_NONE;
		SelectedKey = INDEX_NONE;
		SelectedChannel = 0;
		SelectedKeys.Reset();
	}
	bOverlapSelected = bOverlap && Index > 0;
	OnSelectionChangedDelegate.ExecuteIfBound();
}

void SAnimStitchTimeline::ToggleTrackExpanded(int32 Track)
{
	if (!Asset->Timeline.Tracks.IsValidIndex(Track)) { return; }
	const FString Key = TrackKey(Asset->Timeline.Tracks[Track]);
	if (ExpandedTracks.Contains(Key)) { ExpandedTracks.Remove(Key); } else { ExpandedTracks.Add(Key); }
}

void SAnimStitchTimeline::ExpandAllTracks()
{
	for (const FAnimStitchTrack& Track : Asset->Timeline.Tracks) { ExpandedTracks.Add(TrackKey(Track)); }
}

void SAnimStitchTimeline::CollapseAllTracks()
{
	ExpandedTracks.Reset();
	VScroll = 0.f;
}

bool SAnimStitchTimeline::AddKeyAtPlayhead()
{
	TArray<int32> Targets;
	if (Asset->Timeline.Tracks.IsValidIndex(SelectedTrack)) { Targets.AddUnique(SelectedTrack); }
	for (const FKeyRef& Ref : SelectedKeys) { if (Asset->Timeline.Tracks.IsValidIndex(Ref.Track)) { Targets.AddUnique(Ref.Track); } }
	if (Targets.Num() == 0) { return false; }
	const FScopedTransaction Tx(LOCTEXT("AddKeyAtPlayheadTx", "Add Key at Playhead"));
	for (const int32 TrackIndex : Targets)
	{
		Asset->SetTrackChannelKey(TrackIndex, MakeKeyAtPlayhead(TrackIndex), FAnimStitchManualEdits::GetEditableChannels(Asset->Timeline.Tracks[TrackIndex])); // locked channels are not keyed
	}
	return true;
}

void SAnimStitchTimeline::JumpToKey(int32 Track, int32 Direction)
{
	if (!Asset->Timeline.Tracks.IsValidIndex(Track)) { return; }
	const TArray<FAnimStitchTrackKey>& Keys = Asset->Timeline.Tracks[Track].Keys;
	constexpr double Eps = 1e-4;
	int32 Target = INDEX_NONE;
	if (Direction > 0)
	{
		for (int32 k = 0; k < Keys.Num(); ++k) { if (Keys[k].Time > Playhead + Eps) { Target = k; break; } }
	}
	else
	{
		for (int32 k = Keys.Num() - 1; k >= 0; --k) { if (Keys[k].Time < Playhead - Eps) { Target = k; break; } }
	}
	SelectedTrack = Track; ++TrackSelectionSerial;
	if (Target == INDEX_NONE) { return; }
	SelectedKey = Target;
	SelectedChannel = 0;
	SelectedKeys.Reset();
	SelectedKeys.Add(FKeyRef{ Track, Target });
	SelectClip(INDEX_NONE, false);
	Playhead = FMath::Clamp(Keys[Target].Time, 0.0, Asset->Timeline.GetTotalLength());
	OnPlayheadChangedDelegate.ExecuteIfBound();
}

bool SAnimStitchTimeline::GetLaneButton(float X, float YInLane, int32 Track, EHit& OutType) const
{
	// In the track column: an expand arrow at the left; previous key, add key, next key at the right.
	if (X < 24.f) { OutType = EHit::TrackExpand; return true; }
	const float Left = LabelWidth - 4.f - 3 * 24.f;
	if (X < Left || X > LabelWidth - 4.f || YInLane < 4.f || YInLane > TrackLaneHeight - 4.f) { return false; }
	const int32 Which = FMath::Clamp(int32((X - Left) / 24.f), 0, 2);
	OutType = Which == 0 ? EHit::TrackPrev : (Which == 1 ? EHit::TrackAddKey : EHit::TrackNext);
	return true;
}

void SAnimStitchTimeline::GetVisibleTimeRange(double& OutStart, double& OutEnd) const
{
	const double Width = FMath::Max(GetCachedGeometry().GetLocalSize().X - LabelWidth, 50.f);
	OutStart = ViewStart;
	OutEnd = ViewStart + Width / PxPerSec;
}

void SAnimStitchTimeline::SetVisibleTimeRange(double Start, double End)
{
	const double Width = FMath::Max(GetCachedGeometry().GetLocalSize().X - LabelWidth, 50.f);
	const double Span = FMath::Max(End - Start, 0.05);
	PxPerSec = FMath::Clamp(Width / Span, 10.0, 4000.0);
	ViewStart = Start;
}

double SAnimStitchTimeline::GetDisplayFps() const
{
	for (const FAnimStitchClip& Clip : Asset->Timeline.Clips)
	{
		if (Clip.Sequence)
		{
			const double Fps = Clip.Sequence->GetSamplingFrameRate().AsDecimal();
			if (Fps > 1.0) { return Fps; }
		}
	}
	return 30.0;
}

bool SAnimStitchTimeline::JumpToAdjacentKey(int32 Direction)
{
	constexpr double Eps = 1e-4;
	bool bFound = false;
	double Best = 0.0;
	auto Consider = [&](const FAnimStitchTrack& Track)
	{
		for (const FAnimStitchTrackKey& Key : Track.Keys)
		{
			const bool bOk = Direction > 0 ? Key.Time > Playhead + Eps : Key.Time < Playhead - Eps;
			if (bOk && (!bFound || (Direction > 0 ? Key.Time < Best : Key.Time > Best))) { Best = Key.Time; bFound = true; }
		}
	};
	if (Asset->Timeline.Tracks.IsValidIndex(SelectedTrack))
	{
		Consider(Asset->Timeline.Tracks[SelectedTrack]);
	}
	else
	{
		for (const FAnimStitchTrack& Track : Asset->Timeline.Tracks) { Consider(Track); }
	}
	if (!bFound) { return false; }
	Playhead = FMath::Clamp(Best, 0.0, Asset->Timeline.GetTotalLength());
	OnPlayheadChangedDelegate.ExecuteIfBound();
	return true;
}
void SAnimStitchTimeline::ClampSelection()
{
	SelectedKeys.Reset(); // key indices may have shifted
	{
		TSet<FString> Valid;
		for (const FAnimStitchTrack& Track : Asset->Timeline.Tracks) { Valid.Add(TrackKey(Track)); }
		ExpandedTracks = ExpandedTracks.Intersect(Valid); // tracks that no longer exist drop their expanded state
	}
	if (!Asset->Timeline.Tracks.IsValidIndex(SelectedTrack)) { SelectedTrack = INDEX_NONE; SelectedKey = INDEX_NONE; SelectedChannel = 0; }
	VScroll = FMath::Clamp(VScroll, 0.f, GetTrackScrollMax());
	SelectedClipIds.RemoveAll([this](const FGuid& Id) { return !Asset->Timeline.Clips.ContainsByPredicate([&Id](const FAnimStitchClip& C) { return C.ClipId == Id; }); });
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
		return FReply::Handled().PreventThrottling().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
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
	case EHit::Splitter:
		DragMode = EDrag::Splitter;
		break;
	case EHit::TrackPrev:
	case EHit::TrackNext:
		JumpToKey(Hit.Index, Hit.Type == EHit::TrackNext ? 1 : -1);
		return FReply::Handled();
	case EHit::TrackExpand:
		SelectedTrack = Hit.Index; ++TrackSelectionSerial;
		ToggleTrackExpanded(Hit.Index);
		return FReply::Handled();
	case EHit::TrackAddKey:
	{
		SelectedTrack = Hit.Index; ++TrackSelectionSerial;
		const FScopedTransaction Tx(LOCTEXT("AddKeyTx", "Add Key"));
		Asset->SetTrackChannelKey(Hit.Index, MakeKeyAtPlayhead(Hit.Index), FAnimStitchManualEdits::GetEditableChannels(Asset->Timeline.Tracks[Hit.Index])); // locked channels are not keyed
		SelectedKey = FAnimStitchManualEdits::FindKey(Asset->Timeline.Tracks[Hit.Index], Playhead);
		SelectedChannel = 0;
		return FReply::Handled();
	}
	case EHit::TrackLane:
	case EHit::TrackKey:
		SelectClip(INDEX_NONE, false);
		if (Hit.Type == EHit::TrackKey)
		{
			const bool bAdd = Event.IsControlDown() || Event.IsShiftDown();
			const bool bAlready = IsKeySelected(Hit.Index, Hit.Sub, Hit.Channel);
			if (bAdd)
			{
				if (bAlready) { SelectedKeys.Remove(FKeyRef{ Hit.Index, Hit.Sub, Hit.Channel }); if (SelectedTrack == Hit.Index && SelectedKey == Hit.Sub && SelectedChannel == Hit.Channel) { SelectedKey = INDEX_NONE; SelectedChannel = 0; } }
				else { SelectedKeys.AddUnique(FKeyRef{ Hit.Index, Hit.Sub, Hit.Channel }); }
			}
			else if (!bAlready || SelectedKeys.Num() <= 1)
			{
				SelectedKeys.Reset();
				SelectedKeys.Add(FKeyRef{ Hit.Index, Hit.Sub, Hit.Channel });
			}
			SelectedTrack = Hit.Index; ++TrackSelectionSerial;
			if (!bAdd || !bAlready) { SelectedKey = Hit.Sub; SelectedChannel = Hit.Channel; }
			// Dragging moves a single key; with several keys selected the drag is ignored (copy / paste and delete work on all of them).
			// A locked channel's keys stay selectable (the Details panel shows them) but cannot be dragged.
			const FAnimStitchTrack& HitTrack = TL.Tracks[Hit.Index];
			const uint8 Movable = (Hit.Channel ? Hit.Channel : HitTrack.Keys[Hit.Sub].Channels) & FAnimStitchManualEdits::GetEditableChannels(HitTrack);
			if (SelectedKeys.Num() <= 1 && Movable != 0)
			{
				DragMode = EDrag::KeyMove;
				InitialKeyTime = HitTrack.Keys[Hit.Sub].Time;
			}
		}
		else
		{
			// Empty lane space: select the track and start a marquee.
				SelectedChannel = 0;
			SelectedTrack = Hit.Index; ++TrackSelectionSerial;
			SelectedKey = INDEX_NONE;
			if (Local.X >= LabelWidth)
			{
				MarqueeInitial = (Event.IsControlDown() || Event.IsShiftDown()) ? SelectedKeys : TArray<FKeyRef>();
				SelectedKeys = MarqueeInitial;
				DragMode = EDrag::Marquee;
				MarqueeStart = MarqueeCurrent = Local;
			}
		}
		break;
	case EHit::FootHandle:
	{
		DragMode = EDrag::FootHandle;
		DragFoot = Hit.Index;
		const TArray<FAnimStitchFootLockResult::FEventMark>& Marks = Plants->Events[Hit.Index];
		FootHandleTime = Marks[Hit.Sub].Time;
		constexpr double Gap = 0.04;
		FootHandleMin = Marks.IsValidIndex(Hit.Sub - 1) ? Marks[Hit.Sub - 1].Time + Gap : 0.0;
		FootHandleMax = Marks.IsValidIndex(Hit.Sub + 1) ? Marks[Hit.Sub + 1].Time - Gap : TL.GetTotalLength();
		break;
	}
	case EHit::FootRegion:
	{
		DragMode = EDrag::FootRegion;
		DragFoot = Hit.Index;
		FootDragStartTime = FootDragCurTime = XToTime(Local.X); // a plain click still toggles the plant
		const TArray<FAnimStitchFootLockResult::FEventMark>& Marks = Plants->Events[Hit.Index];
		constexpr double Gap = 0.04;
		FootAnchor = XToTime(Local.X);
		FootRegionStart = Marks[Hit.Sub].Time;
		bFootRegionHasEnd = Marks.IsValidIndex(Hit.Sub + 1);
		FootRegionEnd = bFootRegionHasEnd ? Marks[Hit.Sub + 1].Time : FootRegionStart;
		FootRegionMin = Marks.IsValidIndex(Hit.Sub - 1) ? Marks[Hit.Sub - 1].Time + Gap : 0.0;
		FootRegionMax = Marks.IsValidIndex(Hit.Sub + 2) ? Marks[Hit.Sub + 2].Time - Gap : TL.GetTotalLength();
		break;
	}
	case EHit::FootLane:
		DragMode = EDrag::FootLane;
		DragFoot = Hit.Index;
		FootDragStartTime = FootDragCurTime = XToTime(Local.X);
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
		bCollapseOnRelease = false;
		if (Hit.Type != EHit::Overlap && Event.IsShiftDown())
		{
			// Shift+click adds the clip to (or removes it from) the multi-selection.
			ToggleClipInSelection(Hit.Index);
			return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
		}
		else if (Hit.Type == EHit::Body && GetNumSelectedClips() > 1 && IsClipSelected(Hit.Index))
		{
			// A plain press on one of several selected clips keeps the group so it can be dragged together; releasing without a drag selects just this clip.
			SelectedClip = Hit.Index;
			bCollapseOnRelease = true;
			OnSelectionChangedDelegate.ExecuteIfBound();
		}
		else
		{
			SelectClip(Hit.Index, Hit.Type == EHit::Overlap);
		}
		const FAnimStitchClip& Clip = TL.Clips[Hit.Index];
		InitialTrimIn = Clip.TrimIn;
		InitialTrimOut = Clip.TrimOut;
		InitialOverlap = Clip.OverlapIn;
		InitialStart = Clip.StartTime;
InitialRate = Clip.PlayRate;
		InitialWidth = Clip.GetDuration();
		DragMode = Hit.Type == EHit::Body ? EDrag::Move : Hit.Type == EHit::TrimLeft ? EDrag::TrimLeft : Hit.Type == EHit::TrimRight ? EDrag::TrimRight : EDrag::Overlap;
		// Alt + edge drag stretches the clip (changes its play rate) instead of trimming it.
		if (FAnimStitchShortcuts::ModifiersMatch(EAnimStitchShortcut::RateStretch, Event))
		{
			if (DragMode == EDrag::TrimLeft) { DragMode = EDrag::RateLeft; }
			else if (DragMode == EDrag::TrimRight) { DragMode = EDrag::RateRight; }
		}
		break;
	}
	default:
		if (Local.X < LabelWidth)
		{
			// The track column does not scrub; empty space in it deselects like empty space anywhere else in the timeline.
			if (!Event.IsControlDown() && !Event.IsShiftDown()) { DeselectAll(); }
			return FReply::Handled();
		}
		if (Local.Y > GetTracksTop() && TL.Tracks.Num() > 0)
		{
			// Below the clip lane, among the correction lanes: drag a rectangle to select keys. A plain click on empty space deselects first.
			if (!Event.IsControlDown() && !Event.IsShiftDown()) { DeselectAll(); }
			MarqueeInitial = (Event.IsControlDown() || Event.IsShiftDown()) ? SelectedKeys : TArray<FKeyRef>();
			SelectedKeys = MarqueeInitial;
			SelectedKey = INDEX_NONE;
			SelectedChannel = 0;
			DragMode = EDrag::Marquee;
			MarqueeStart = MarqueeCurrent = Local;
			break;
		}
		// Empty space deselects and also scrubs, so the playhead can be dragged from anywhere.
		DeselectAll();
		DragMode = EDrag::Scrub;
		Playhead = FMath::Clamp(XToTime(Local.X), 0.0, TL.GetTotalLength());
		OnPlayheadChangedDelegate.ExecuteIfBound();
		break;
	}
	return FReply::Handled().PreventThrottling().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SAnimStitchTimeline::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const FVector2f LocalPos = G.AbsoluteToLocal(Event.GetScreenSpacePosition());
		const FHit Hit = HitTest(LocalPos);
		if (Hit.Type == EHit::TrackLane || Hit.Type == EHit::TrackKey)
		{
			SelectedTrack = Hit.Index; ++TrackSelectionSerial;
			SelectedKey = Hit.Type == EHit::TrackKey ? Hit.Sub : INDEX_NONE;
			SelectedChannel = Hit.Type == EHit::TrackKey ? Hit.Channel : 0;
			OpenTrackMenu(Event, Hit.Index, SelectedKey, XToTime(LocalPos.X), Hit.Channel);
			return FReply::Handled();
		}
		if (Hit.Type == EHit::None)
		{
			OpenAddTrackMenu(Event); // right-click on empty space: only the add-bone-track commands
			return FReply::Handled();
		}
		if (Hit.Type == EHit::FootLane || Hit.Type == EHit::FootHandle || Hit.Type == EHit::FootRegion)
		{
			OpenFootLaneMenu(Event, Hit.Index, XToTime(G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X));
			return FReply::Handled();
		}
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
	if (DragMode == EDrag::FootRegion && !bDragStarted)
	{
		ToggleFootPlantAt(DragFoot, FootDragStartTime);
	}
	if (DragMode == EDrag::FootLane)
	{
		if (bDragStarted)
		{
			const FScopedTransaction Tx(LOCTEXT("PinPlantTx", "Pin Foot Plant"));
			Asset->PaintFootLock(DragFoot, FMath::Min(FootDragStartTime, FootDragCurTime), FMath::Max(FootDragStartTime, FootDragCurTime));
		}
		else
		{
			ToggleFootPlantAt(DragFoot, FootDragStartTime);
		}
	}
	if (bCollapseOnRelease && DragMode == EDrag::Move && !bDragStarted && Asset->Timeline.Clips.IsValidIndex(DragIndex)) { SelectClip(DragIndex, false); }
	bCollapseOnRelease = false;
	DragTransaction.Reset(); // ends the undo transaction
	DragMode = EDrag::None;
	bDragStarted = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FText SAnimStitchTimeline::GetHoverTipText() const
{
	if (!Asset || HoverLocal.X < 0.f) { return FText::GetEmpty(); }
	const FHit Hit = HitTest(HoverLocal);
	switch (Hit.Type)
	{
	case EHit::Ruler: return LOCTEXT("TipRuler", "Click or drag to move the playhead");
	case EHit::MarkerStart: return LOCTEXT("TipMarkerStart", "Range START flag (green): playback and bake start here. Drag to move it.");
	case EHit::MarkerEnd: return LOCTEXT("TipMarkerEnd", "Range END flag (red): playback and bake stop here. Drag to move it.");
	case EHit::Splitter: return LOCTEXT("TipSplitter", "Drag to resize the track column");
	case EHit::TrimLeft:
	case EHit::TrimRight: return FText::Format(LOCTEXT("TipTrim", "Drag to trim the clip (non-destructive). {0} + drag stretches it instead (changes its play rate)."), FAnimStitchShortcuts::GetKeysText(EAnimStitchShortcut::RateStretch));
	case EHit::Overlap: return LOCTEXT("TipOverlap", "Crossfade between two clips: drag to change its length. Pull clips apart for a gap, which crossfades too.");
	case EHit::Body: return LOCTEXT("TipClip", "Clip: click to select, drag to move or reorder it. Right-click for more.");
	case EHit::FootLane: return LOCTEXT("TipFootLane", "Foot lock lane: click to lock or unlock this foot at that moment, drag to lock a stretch, right-click for more.");
	case EHit::FootHandle: return LOCTEXT("TipFootHandle", "Edge of a manual foot lock: drag to move it");
	case EHit::FootRegion: return LOCTEXT("TipFootRegion", "Manual foot lock: drag to move the whole region, click to unlock");
	case EHit::TrackPrev: return LOCTEXT("TipPrevKey", "Jump to the previous key of this track");
	case EHit::TrackNext: return LOCTEXT("TipNextKey", "Jump to the next key of this track");
	case EHit::TrackAddKey: return FText::Format(LOCTEXT("TipAddKey", "Add a key at the playhead ({0}). Bone tracks key only their unlocked channels."), FAnimStitchShortcuts::GetKeysText(EAnimStitchShortcut::AddKey));
	case EHit::TrackExpand: return LOCTEXT("TipExpand", "Expand or collapse the track: separate Translation, Rotation and Scale key lanes with lock toggles, and numeric values");
	case EHit::TrackKey:
		if (Hit.Channel != 0 && Asset->Timeline.Tracks.IsValidIndex(Hit.Index) && !IsChannelEditable(Hit.Index, Hit.Channel))
		{
			return LOCTEXT("TipKeyLocked", "Key on a locked channel lane: its value cannot be edited or moved while the lane is locked, but Delete still removes it.");
		}
		return LOCTEXT("TipKey", "Key: click to select, drag to move it, Delete removes it, right-click for more");
	case EHit::TrackLane: return LOCTEXT("TipTrackLane", "Correction track: click to select it, drag a rectangle to select keys, right-click for more");
	default: return FText::GetEmpty();
	}
}

FReply SAnimStitchTimeline::OnMouseMove(const FGeometry& G, const FPointerEvent& Event)
{
	HoverLocal = G.AbsoluteToLocal(Event.GetScreenSpacePosition());
	if (!HasMouseCapture() || DragMode == EDrag::None)
	{
		return FReply::Unhandled();
	}
	if (DragMode == EDrag::Marquee)
	{
		UpdateMarquee(G.AbsoluteToLocal(Event.GetScreenSpacePosition()));
		return FReply::Handled();
	}
	OnDragMove(G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X);
	return FReply::Handled();
}

void SAnimStitchTimeline::OnDragMove(float LocalX)
{
	FAnimStitchTimeline& TL = Asset->Timeline;

	if (DragMode == EDrag::FootRegion)
	{
		if (!bDragStarted)
		{
			if (FMath::Abs(LocalX - DragStartX) < 3.f) { return; }
			bDragStarted = true;
			DragTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("MoveLockTx", "Move Foot Lock"));
		}
		const double Length = bFootRegionHasEnd ? FootRegionEnd - FootRegionStart : 0.0;
		double NewStart = FootRegionStart + (XToTime(LocalX) - FootAnchor);
		NewStart = FMath::Clamp(NewStart, FootRegionMin, FMath::Max(FootRegionMin, FootRegionMax - Length));
		const double Shift = NewStart - FootRegionStart;
		if (!FMath::IsNearlyZero(Shift, 1e-6))
		{
			// The end moves first when going right, the start first when going left, so the two never cross.
			if (bFootRegionHasEnd && Shift > 0.0) { Asset->MoveFootEvent(DragFoot, FootRegionEnd, FootRegionEnd + Shift); }
			Asset->MoveFootEvent(DragFoot, FootRegionStart, FootRegionStart + Shift);
			if (bFootRegionHasEnd && Shift < 0.0) { Asset->MoveFootEvent(DragFoot, FootRegionEnd, FootRegionEnd + Shift); }
			FootRegionStart += Shift;
			FootRegionEnd += Shift;
			FootAnchor += Shift;
		}
		return;
	}
	if (DragMode == EDrag::FootHandle)
	{
		if (!bDragStarted)
		{
			if (FMath::Abs(LocalX - DragStartX) < 3.f) { return; }
			bDragStarted = true;
			DragTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("ResizeLockTx", "Resize Foot Lock"));
		}
		const double NewTime = FMath::Clamp(XToTime(LocalX), FootHandleMin, FMath::Max(FootHandleMin, FootHandleMax));
		if (!FMath::IsNearlyEqual(NewTime, FootHandleTime, 1e-6))
		{
			Asset->MoveFootEvent(DragFoot, FootHandleTime, NewTime);
			FootHandleTime = NewTime;
		}
		return;
	}
	if (DragMode == EDrag::Splitter)
	{
		LabelWidth = FMath::Clamp(LocalX, 260.f, 620.f);
		return;
	}
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
	if (DragMode == EDrag::KeyMove)
	{
		if (!bDragStarted)
		{
			if (FMath::Abs(LocalX - DragStartX) < 3.f) { return; }
			bDragStarted = true;
			DragTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("MoveKeyTx", "Move Key"));
		}
		SelectedKey = Asset->MoveTrackChannelKey(SelectedTrack, SelectedKey, SelectedChannel ? SelectedChannel : AnimStitchChannel::All, InitialKeyTime + (LocalX - DragStartX) / PxPerSec);
		SelectedKeys.Reset();
		SelectedKeys.Add(FKeyRef{ SelectedTrack, SelectedKey, SelectedChannel });
		return;
	}
	if (DragMode == EDrag::FootLane)
	{
		FootDragCurTime = XToTime(LocalX);
		bDragStarted = FMath::Abs(LocalX - DragStartX) >= 3.f;
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
	// Trim is a pure in/out point at the original rate; the handle moves Dt of timeline time = Dt * rate of source, both directions.
	Asset->SetTrim(DragIndex, InitialTrimIn, InitialTrimOut + Dt * InitialRate);
	break;
case EDrag::RateRight:
{
	// Same content, new width: the rate follows the dragged edge.
	const double Width = FMath::Max(InitialWidth + Dt, 1e-3);
	Asset->SetClipRate(DragIndex, (InitialTrimOut - InitialTrimIn) / Width);
	break;
}
case EDrag::RateLeft:
{
	const double Width = FMath::Max(InitialWidth - Dt, 1e-3);
	Asset->SetClipRate(DragIndex, (InitialTrimOut - InitialTrimIn) / Width);
	if (DragIndex > 0)
	{
		// Keep the clip's end where it was: the dragged start edge follows the cursor (as far as the rate limits allow).
		const double NewWidth = TL.Clips[DragIndex].GetDuration();
		Asset->SetOverlap(DragIndex, TL.Clips[DragIndex - 1].GetEndTime() - (InitialStart + InitialWidth - NewWidth));
	}
	break;
}
	case EDrag::TrimLeft:
	{
Asset->SetTrim(DragIndex, InitialTrimIn + Dt * InitialRate, InitialTrimOut);
		// Keep the dragged edge under the cursor where an overlap allows it; otherwise the clip ripples.
const double Applied = (TL.Clips[DragIndex].TrimIn - InitialTrimIn) / InitialRate;
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
		if (GetNumSelectedClips() > 1)
		{
			// Several clips selected: they move together as one block (their relative order is kept).
			const TArray<int32> Block = GetSelectedClipIndices();
			const FGuid DragId = TL.Clips[DragIndex].ClipId;
			const double GroupCenter = InitialStart + Dt + TL.Clips[DragIndex].GetDuration() * 0.5;
			int32 Current = 0, Target = 0;
			for (int32 i = 0; i < TL.Clips.Num(); ++i)
			{
				if (Block.Contains(i)) { continue; }
				if (i < DragIndex) { ++Current; }
				if (TL.Clips[i].StartTime + TL.Clips[i].GetDuration() * 0.5 < GroupCenter) { ++Target; }
			}
			Target = FMath::Clamp(Target, Current - 1, Current + 1); // one step per mouse event, like a single clip
			if (Target != Current)
			{
				Asset->MoveClipBlock(Block, Target);
				const int32 NewIndex = TL.Clips.IndexOfByPredicate([&DragId](const FAnimStitchClip& C) { return C.ClipId == DragId; });
				if (NewIndex != INDEX_NONE) { DragIndex = NewIndex; SelectedClip = NewIndex; }
			}
			break;
		}
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
	const float WidgetX = G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
	if (WidgetX < LabelWidth)
	{
		// Over the track column the wheel scrolls the lanes.
		SetTrackScroll(VScroll - Event.GetWheelDelta() * 40.f);
		return FReply::Handled();
	}
	const float Local = FMath::Max(WidgetX - LabelWidth, 0.f);
	if (Event.IsShiftDown())
	{
		ViewStart -= Event.GetWheelDelta() * 60.0 / PxPerSec;
	}
	else
	{
		// Wheel zooms around the cursor, like Sequencer.
		const double AnchorTime = ViewStart + Local / PxPerSec;
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
			: (DragMode == EDrag::MarkerStart || DragMode == EDrag::MarkerEnd || DragMode == EDrag::Splitter || DragMode == EDrag::FootHandle) ? EHit::MarkerStart : DragMode == EDrag::FootRegion ? EHit::Body
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
	case EHit::Splitter:
	case EHit::FootHandle:
		return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
	case EHit::Body:
	case EHit::FootRegion:
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
	if (!Event.IsRepeat() && FAnimStitchShortcuts::Matches(EAnimStitchShortcut::AddKey, Event) && AddKeyAtPlayhead()) { return FReply::Handled(); }
	if (FAnimStitchShortcuts::Matches(EAnimStitchShortcut::CopyKeys, Event)) { CopySelectedKeys(); return FReply::Handled(); }
	if (FAnimStitchShortcuts::Matches(EAnimStitchShortcut::PasteKeys, Event)) { PasteKeys(); return FReply::Handled(); }
	if (SelectedKeys.Num() > 1 && (Event.GetKey() == EKeys::Delete || Event.GetKey() == EKeys::BackSpace)) { DeleteSelectedKeys(); return FReply::Handled(); }
	if ((Event.GetKey() == EKeys::Delete || Event.GetKey() == EKeys::BackSpace) && Asset->Timeline.Tracks.IsValidIndex(SelectedTrack) && Asset->Timeline.Tracks[SelectedTrack].Keys.IsValidIndex(SelectedKey))
	{
		const FScopedTransaction Tx(LOCTEXT("DeleteKeyTx", "Delete Key"));
		Asset->RemoveTrackChannelKey(SelectedTrack, SelectedKey, SelectedChannel ? SelectedChannel : AnimStitchChannel::All);
		SelectedKey = INDEX_NONE;
		SelectedChannel = 0;
		return FReply::Handled();
	}
	if ((Event.GetKey() == EKeys::Delete || Event.GetKey() == EKeys::BackSpace) && Asset->Timeline.Tracks.IsValidIndex(SelectedTrack) && !Asset->Timeline.Clips.IsValidIndex(SelectedClip))
	{
		// A whole track is selected: delete the track.
		const FScopedTransaction Tx(LOCTEXT("DeleteTrackTx", "Delete Track"));
		Asset->RemoveTrack(SelectedTrack);
		SelectedTrack = INDEX_NONE;
		SelectedKey = INDEX_NONE;
		SelectedKeys.Reset();
		return FReply::Handled();
	}
	if (Event.GetKey() == EKeys::Delete || Event.GetKey() == EKeys::BackSpace)
	{
		DeleteSelected();
		return FReply::Handled();
	}
	if (FAnimStitchShortcuts::Matches(EAnimStitchShortcut::DuplicateClip, Event))
	{
		DuplicateSelected();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SAnimStitchTimeline::DeleteSelected()
{
	if (GetNumSelectedClips() > 1)
	{
		const FScopedTransaction Tx(LOCTEXT("RemoveClipsTx", "Remove Clips"));
		Asset->RemoveClips(GetSelectedClipIndices());
		ClampSelection();
		SelectClip(INDEX_NONE, false);
	}
	else if (Asset->Timeline.Clips.IsValidIndex(SelectedClip))
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

void SAnimStitchTimeline::ToggleFootPlantAt(int32 Foot, double Time)
{
	if (!Plants.IsValid() || !Plants->bValid || !Plants->Mask[Foot].Num())
	{
		return;
	}
	// Click toggles the foot at this moment: locked -> unlock, otherwise lock. The state persists forward until the next click.
	const TArray<uint8>& Mask = Plants->Mask[Foot];
	const int32 Frame = FMath::Clamp(FMath::RoundToInt(Time * Plants->Fps), 0, Mask.Num() - 1);
	const bool bLocked = AnimStitchPlant::IsPlanted(Mask[Frame]);
	const FScopedTransaction Tx(bLocked ? LOCTEXT("UnlockFootTx", "Unlock Foot") : LOCTEXT("LockFootTx", "Lock Foot"));
	Asset->AddFootEvent(Foot, Time, bLocked ? EAnimStitchFootAction::Unlock : EAnimStitchFootAction::Lock);
}

void SAnimStitchTimeline::OpenFootLaneMenu(const FPointerEvent& Event, int32 Foot, double Time)
{
	FMenuBuilder Menu(true, nullptr);
	auto AddAction = [this, &Menu, Foot, Time](const FText& Label, const FText& Tip, EAnimStitchFootAction Action)
	{
		Menu.AddMenuEntry(Label, Tip, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Foot, Time, Action]()
		{
			const FScopedTransaction Tx(LOCTEXT("FootEventTx", "Foot Lock Event"));
			Asset->AddFootEvent(Foot, Time, Action);
		})));
	};
	AddAction(LOCTEXT("LockHere", "Lock Foot Here"), LOCTEXT("LockHereTip", "Glue this foot to its world position here until you unlock it"), EAnimStitchFootAction::Lock);
	AddAction(LOCTEXT("UnlockHere", "Unlock Foot Here"), LOCTEXT("UnlockHereTip", "Release this foot here; it stays free until you lock it again"), EAnimStitchFootAction::Unlock);
	AddAction(LOCTEXT("AutoHere", "Resume Auto-Detection Here"), LOCTEXT("AutoHereTip", "From here on, plants are detected automatically again"), EAnimStitchFootAction::Auto);
	Menu.AddSeparator();
	auto HasFootEvents = [this](int32 FootOrAll) { for (const FAnimStitchClip& C : Asset->Timeline.Clips) { for (const FAnimStitchFootEvent& E : C.FootEvents) { if (FootOrAll == INDEX_NONE || E.Foot == FootOrAll) { return true; } } } return false; };
	Menu.AddMenuEntry(LOCTEXT("ClearAllFootEvents", "Clear All Footlocks"), LOCTEXT("ClearAllFootEventsTip", "Remove every manual lock / unlock mark on both feet: back to automatic detection"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]()
		{
			const FScopedTransaction Tx(LOCTEXT("ClearAllFootTx", "Clear All Footlocks"));
			Asset->ClearFootEvents(INDEX_NONE);
		}), FCanExecuteAction::CreateLambda([HasFootEvents]() { return HasFootEvents(INDEX_NONE); })));
	Menu.AddMenuEntry(LOCTEXT("ClearFootEvents", "Clear Footlocks (This Foot)"), LOCTEXT("ClearFootEventsTip", "Remove the manual lock / unlock marks of this foot only"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Foot]()
		{
			const FScopedTransaction Tx(LOCTEXT("ClearFootTx", "Clear Footlocks"));
			Asset->ClearFootEvents(Foot);
		}), FCanExecuteAction::CreateLambda([HasFootEvents, Foot]() { return HasFootEvents(Foot); })));
	const FWidgetPath Path = Event.GetEventPath() != nullptr ? *Event.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), Path, Menu.MakeWidget(), Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
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
	const FCanExecuteAction HasSel = FCanExecuteAction::CreateLambda([this]() { return Asset->Timeline.Clips.IsValidIndex(SelectedClip); });
	Menu.AddMenuEntry(LOCTEXT("CtxRootAuthority", "Root Motion Authority"), LOCTEXT("CtxRootAuthorityTip", "The baked root track follows this clip; other clips continue from where it leaves off"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]()
		{
			const FScopedTransaction Tx(LOCTEXT("SetRootAuthTx", "Set Root Motion Authority"));
			Asset->SetRootAuthority(SelectedClip);
		}), HasSel, FIsActionChecked::CreateLambda([this]() { return SelectedClip == Asset->Timeline.RootAuthorityIndex; })), NAME_None, EUserInterfaceActionType::Check);
	Menu.AddMenuEntry(LOCTEXT("CtxAddCrossfade", "Add Crossfade (default length)"), LOCTEXT("CtxAddCrossfadeTip", "Overlap this clip with the previous one using the default length and curve from the AnimStitch settings"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]()
		{
			const FScopedTransaction Tx(LOCTEXT("AddCrossfadeTx", "Add Crossfade"));
			Asset->AddDefaultCrossfade(SelectedClip);
		}), FCanExecuteAction::CreateLambda([this]() { return SelectedClip > 0 && Asset->Timeline.Clips.IsValidIndex(SelectedClip); })));
	Menu.AddMenuEntry(LOCTEXT("CtxBatchSlot", "Batch Slot"), LOCTEXT("CtxBatchSlotTip", "In Batch Bake this slot is replaced by each of your source clips"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]()
		{
			const FScopedTransaction Tx(LOCTEXT("SetBatchSlotTx", "Toggle Batch Slot"));
			Asset->SetBatchSlot(SelectedClip, !Asset->Timeline.Clips[SelectedClip].bBatchSlot);
		}), HasSel, FIsActionChecked::CreateLambda([this]() { return Asset->Timeline.Clips.IsValidIndex(SelectedClip) && Asset->Timeline.Clips[SelectedClip].bBatchSlot; })), NAME_None, EUserInterfaceActionType::Check);
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

void SAnimStitchTimeline::ZoomToRange()
{
	double RangeStart, RangeEnd;
	Asset->GetPlaybackRange(RangeStart, RangeEnd);
	constexpr double Eps = 1e-4;
	if (RangeStart <= Eps && RangeEnd >= Asset->Timeline.GetTotalLength() - Eps)
	{
		ZoomToFit(); // the range is the whole timeline
		return;
	}
	SetVisibleTimeRange(RangeStart, RangeEnd); // flag to flag, edge to edge
}

void SAnimStitchTimeline::SetRangeFlagToPlayhead(bool bStart)
{
	double RangeStart, RangeEnd;
	Asset->GetPlaybackRange(RangeStart, RangeEnd);
	const FScopedTransaction Tx(bStart ? LOCTEXT("SetRangeStartTx", "Set Range Start to Playhead") : LOCTEXT("SetRangeEndTx", "Set Range End to Playhead"));
	if (bStart) { Asset->SetPlaybackRange(Playhead, RangeEnd); }
	else { Asset->SetPlaybackRange(RangeStart, Playhead); }
}

void SAnimStitchTimeline::ZoomToFit()
{
	const double Length = FMath::Max(Asset->Timeline.GetTotalLength(), 0.5);
	const double Width = FMath::Max(GetTickSpaceGeometry().GetLocalSize().X - LabelWidth, 100.f);
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
	AddSequencesAt(Sequences, Index);
	return FReply::Handled();
}

void SAnimStitchTimeline::AddSequencesAt(const TArray<UAnimSequence*>& Sequences, int32 Index)
{
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
			SelectClip(Index == INDEX_NONE ? Asset->Timeline.Clips.Num() - Added : Index, false);
			RequestZoomToFit();
		}
	}
	if (Warnings.Num() > 0)
	{
		OnWarningsDelegate.ExecuteIfBound(Warnings);
	}
}

#undef LOCTEXT_NAMESPACE









