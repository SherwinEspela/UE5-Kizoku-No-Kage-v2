#include "SAnimStitchTransportBar.h"
#include "SAnimStitchTimeline.h"
#include "AnimStitchTimelineAsset.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

// ---------------------------------------------------------------------------------------------
// Range bar
// ---------------------------------------------------------------------------------------------

void SAnimStitchRangeBar::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;
}

void SAnimStitchRangeBar::GetBarRange(double& OutStart, double& OutEnd) const
{
	// Everything the user may want to reach: the content plus some room around it, widened to include the current view.
	double ViewStart, ViewEnd;
	Timeline->GetVisibleTimeRange(ViewStart, ViewEnd);
	const double Length = Timeline->GetAsset()->Timeline.GetTotalLength();
	const double Pad = FMath::Max(1.0, Length * 0.15);
	OutStart = FMath::Min(-0.2, ViewStart);
	OutEnd = FMath::Max(Length + Pad, ViewEnd);
}

SAnimStitchRangeBar::EGrab SAnimStitchRangeBar::HitThumb(float X, float Width) const
{
	double RangeStart, RangeEnd, ViewStart, ViewEnd;
	GetBarRange(RangeStart, RangeEnd);
	Timeline->GetVisibleTimeRange(ViewStart, ViewEnd);
	const double Span = FMath::Max(RangeEnd - RangeStart, 1e-6);
	const float X0 = float((ViewStart - RangeStart) / Span) * Width;
	const float X1 = float((ViewEnd - RangeStart) / Span) * Width;
	constexpr float Edge = 7.f;
	if (X < X0 - 2.f || X > X1 + 2.f) { return EGrab::None; }
	if (X <= X0 + Edge) { return EGrab::Left; }
	if (X >= X1 - Edge) { return EGrab::Right; }
	return EGrab::Body;
}

int32 SAnimStitchRangeBar::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const FVector2f Size = G.GetLocalSize();
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	double RangeStart, RangeEnd, ViewStart, ViewEnd;
	GetBarRange(RangeStart, RangeEnd);
	Timeline->GetVisibleTimeRange(ViewStart, ViewEnd);
	const double Span = FMath::Max(RangeEnd - RangeStart, 1e-6);
	const float X0 = FMath::Clamp(float((ViewStart - RangeStart) / Span) * Size.X, 0.f, Size.X - 4.f);
	const float X1 = FMath::Clamp(float((ViewEnd - RangeStart) / Span) * Size.X, X0 + 4.f, Size.X);
	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size.X, Size.Y - 6.f), FSlateLayoutTransform(FVector2f(0.f, 3.f))), White, ESlateDrawEffect::None, FLinearColor(0.03f, 0.03f, 0.035f));
	FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(X1 - X0, Size.Y - 6.f), FSlateLayoutTransform(FVector2f(X0, 3.f))), White, ESlateDrawEffect::None, FLinearColor(0.32f, 0.32f, 0.36f));
	FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(3.f, Size.Y - 6.f), FSlateLayoutTransform(FVector2f(X0, 3.f))), White, ESlateDrawEffect::None, FLinearColor(0.7f, 0.7f, 0.78f));
	FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(3.f, Size.Y - 6.f), FSlateLayoutTransform(FVector2f(X1 - 3.f, 3.f))), White, ESlateDrawEffect::None, FLinearColor(0.7f, 0.7f, 0.78f));
	// Playhead position as a red tick.
	const double Playhead = Timeline->GetPlayhead();
	const float PX = FMath::Clamp(float((Playhead - RangeStart) / Span) * Size.X, 0.f, Size.X);
	FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(2.f, Size.Y), FSlateLayoutTransform(FVector2f(PX - 1.f, 0.f))), White, ESlateDrawEffect::None, FLinearColor(1.f, 0.25f, 0.25f));
	return Layer + 4;
}

FReply SAnimStitchRangeBar::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton) { return FReply::Unhandled(); }
	const float X = G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
	const float Width = G.GetLocalSize().X;
	GetBarRange(GrabRangeStart, GrabRangeEnd);
	Timeline->GetVisibleTimeRange(GrabStart, GrabEnd);
	Grab = HitThumb(X, Width);
	if (Grab == EGrab::None)
	{
		// Click in the groove: centre the view there.
		const double Span = GrabRangeEnd - GrabRangeStart;
		const double Centre = GrabRangeStart + X / Width * Span;
		const double Half = (GrabEnd - GrabStart) * 0.5;
		Timeline->SetVisibleTimeRange(Centre - Half, Centre + Half);
		GrabStart = Centre - Half;
		GrabEnd = Centre + Half;
		Grab = EGrab::Body;
	}
	GrabX = X;
	return FReply::Handled().CaptureMouse(SharedThis(this)).PreventThrottling();
}

FReply SAnimStitchRangeBar::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton || Grab == EGrab::None) { return FReply::Unhandled(); }
	Grab = EGrab::None;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SAnimStitchRangeBar::OnMouseMove(const FGeometry& G, const FPointerEvent& Event)
{
	if (Grab == EGrab::None || !HasMouseCapture()) { return FReply::Unhandled(); }
	const double Delta = (G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X - GrabX) / FMath::Max(G.GetLocalSize().X, 1.f) * (GrabRangeEnd - GrabRangeStart);
	constexpr double MinSpan = 0.1;
	switch (Grab)
	{
	case EGrab::Left: Timeline->SetVisibleTimeRange(FMath::Min(GrabStart + Delta, GrabEnd - MinSpan), GrabEnd); break;
	case EGrab::Right: Timeline->SetVisibleTimeRange(GrabStart, FMath::Max(GrabEnd + Delta, GrabStart + MinSpan)); break;
	default: Timeline->SetVisibleTimeRange(GrabStart + Delta, GrabEnd + Delta); break;
	}
	return FReply::Handled();
}

FCursorReply SAnimStitchRangeBar::OnCursorQuery(const FGeometry& G, const FPointerEvent& Event) const
{
	const EGrab Where = Grab != EGrab::None ? Grab : HitThumb(G.AbsoluteToLocal(Event.GetScreenSpacePosition()).X, G.GetLocalSize().X);
	if (Where == EGrab::Left || Where == EGrab::Right) { return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight); }
	if (Where == EGrab::Body) { return FCursorReply::Cursor(EMouseCursor::GrabHand); }
	return FCursorReply::Unhandled();
}

// ---------------------------------------------------------------------------------------------
// Transport bar
// ---------------------------------------------------------------------------------------------

FText SAnimStitchTransportBar::FormatTime(double Seconds) const
{
	if (Timeline->GetShowFrames())
	{
		return FText::FromString(FString::Printf(TEXT("%04d"), FMath::RoundToInt(Seconds * Timeline->GetDisplayFps())));
	}
	return FText::FromString(FString::Printf(TEXT("%.2fs"), Seconds));
}

void SAnimStitchTransportBar::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;
	const TFunction<void()> OnGoToStart = InArgs._OnGoToStart, OnPreviousKey = InArgs._OnPreviousKey, OnPlayPause = InArgs._OnPlayPause, OnNextKey = InArgs._OnNextKey,
		OnGoToEnd = InArgs._OnGoToEnd, OnStop = InArgs._OnStop, OnToggleLoop = InArgs._OnToggleLoop;
	const TFunction<void(double)> OnSetPlayhead = InArgs._OnSetPlayhead;
	const TFunction<bool()> IsPlaying = InArgs._IsPlaying, IsLooping = InArgs._IsLooping;

	auto Button = [](TAttribute<const FSlateBrush*> Brush, const FText& Tip, TFunction<void()> OnClick)
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(Tip)
			.ContentPadding(FMargin(5.f, 3.f))
			.OnClicked_Lambda([OnClick]() { if (OnClick) { OnClick(); } return FReply::Handled(); })
			[
				SNew(SImage).Image(Brush).ColorAndOpacity(FSlateColor::UseForeground())
			];
	};
	auto Static = [](const char* Name) { return TAttribute<const FSlateBrush*>(FAppStyle::GetBrush(Name)); };

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(4.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Button(Static("Animation.Backward_End"), LOCTEXT("GoStartTip", "Go to the start of the playback range"), OnGoToStart)]
			+ SHorizontalBox::Slot().AutoWidth()[Button(Static("Animation.Backward_Step"), LOCTEXT("PrevKeyTip", "Previous key (of the selected track, or of all tracks)"), OnPreviousKey)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Button(TAttribute<const FSlateBrush*>::CreateLambda([IsPlaying]() { return FAppStyle::GetBrush(IsPlaying && IsPlaying() ? "Animation.Pause" : "Animation.Forward"); }), LOCTEXT("PlayPauseTip", "Play / Pause (Space)"), OnPlayPause)
			]
			+ SHorizontalBox::Slot().AutoWidth()[Button(Static("Animation.Forward_Step"), LOCTEXT("NextKeyTip", "Next key (of the selected track, or of all tracks)"), OnNextKey)]
			+ SHorizontalBox::Slot().AutoWidth()[Button(Static("Animation.Forward_End"), LOCTEXT("GoEndTip", "Go to the end of the playback range"), OnGoToEnd)]
			+ SHorizontalBox::Slot().AutoWidth()[Button(Static("Animation.Stop"), LOCTEXT("StopTip2", "Stop and return to the start of the playback range"), OnStop)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 8.f, 0.f)
			[
				Button(TAttribute<const FSlateBrush*>::CreateLambda([IsLooping]() { return FAppStyle::GetBrush(IsLooping && IsLooping() ? "Animation.Loop.Enabled" : "Animation.Loop.Disabled"); }), LOCTEXT("LoopTip2", "Toggle looping of the playback range"), OnToggleLoop)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBox).WidthOverride(86.f)
				[
					SNew(SSpinBox<double>).MinValue(0.0).Delta(0.01)
					.ToolTipText(LOCTEXT("TimeEntryTip", "Current time (type a value to jump there)"))
					.MinFractionalDigits(0).MaxFractionalDigits(2)
					.Value_Lambda([this]() { return Timeline->GetShowFrames() ? double(FMath::RoundToInt(Timeline->GetPlayhead() * Timeline->GetDisplayFps())) : Timeline->GetPlayhead(); })
					.OnValueCommitted_Lambda([this, OnSetPlayhead](double V, ETextCommit::Type)
					{
						if (OnSetPlayhead) { OnSetPlayhead(Timeline->GetShowFrames() ? V / Timeline->GetDisplayFps() : V); }
					})
					.OnValueChanged_Lambda([this, OnSetPlayhead](double V)
					{
						if (OnSetPlayhead) { OnSetPlayhead(Timeline->GetShowFrames() ? V / Timeline->GetDisplayFps() : V); }
					})
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock).Text_Lambda([this]() { return FText::Format(LOCTEXT("TotalLength", "/ {0}"), FormatTime(Timeline->GetAsset()->Timeline.GetTotalLength())); })
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SCheckBox).Style(FAppStyle::Get(), "ToggleButtonCheckbox")
				.ToolTipText(LOCTEXT("UnitsTip", "Show the timeline in frames instead of seconds"))
				.IsChecked_Lambda([this]() { return Timeline->GetShowFrames() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Timeline->SetShowFrames(State == ECheckBoxState::Checked); })
				[
					SNew(STextBlock).Text_Lambda([this]() { return Timeline->GetShowFrames() ? LOCTEXT("UnitsFrames", "Frames") : LOCTEXT("UnitsSeconds", "Seconds"); }).Margin(FMargin(8.f, 2.f))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBox).MinDesiredWidth(44.f)
				[
					SNew(STextBlock).Text_Lambda([this]() { double S, E; Timeline->GetVisibleTimeRange(S, E); return FormatTime(S); }).Justification(ETextJustify::Right)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(18.f)
				[
					SNew(SAnimStitchRangeBar).Timeline(Timeline)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox).MinDesiredWidth(44.f)
				[
					SNew(STextBlock).Text_Lambda([this]() { double S, E; Timeline->GetVisibleTimeRange(S, E); return FormatTime(E); })
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
