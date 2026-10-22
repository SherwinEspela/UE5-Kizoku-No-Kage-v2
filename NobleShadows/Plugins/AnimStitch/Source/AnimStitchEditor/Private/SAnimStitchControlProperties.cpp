#include "SAnimStitchControlProperties.h"
#include "SAnimStitchTimeline.h"
#include "AnimStitchPreview.h"
#include "AnimStitchTimelineAsset.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

bool SAnimStitchControlProperties::GetTarget(UAnimStitchTimelineAsset*& OutAsset, int32& OutTrack, bool& bOutPole) const
{
	const FAnimStitchControlId Id = Preview->GetSelectedControl();
	OutAsset = Timeline->GetAsset();
	OutTrack = Id.A;
	bOutPole = Id.Kind == EAnimStitchControlKind::IKPole && Timeline->GetAsset() && Timeline->GetAsset()->Timeline.Tracks.IsValidIndex(Id.A) && Timeline->GetAsset()->Timeline.Tracks[Id.A].Kind != EAnimStitchTrackKind::IKPole; // a pole lane carries its own look
	const bool bTrackControl = Id.Kind == EAnimStitchControlKind::Bone || Id.Kind == EAnimStitchControlKind::IKEffector || Id.Kind == EAnimStitchControlKind::IKPole;
	return bTrackControl && OutAsset && OutAsset->Timeline.Tracks.IsValidIndex(OutTrack);
}

FAnimStitchControlStyle SAnimStitchControlProperties::GetStyle() const
{
	UAnimStitchTimelineAsset* Asset = nullptr;
	int32 Track = INDEX_NONE;
	bool bPole = false;
	if (!GetTarget(Asset, Track, bPole)) { return FAnimStitchControlStyle(); }
	return bPole ? Asset->Timeline.Tracks[Track].PoleStyle : Asset->Timeline.Tracks[Track].ControlStyle;
}

void SAnimStitchControlProperties::Edit(TFunctionRef<void(FAnimStitchControlStyle&)> Mutator)
{
	UAnimStitchTimelineAsset* Asset = nullptr;
	int32 Track = INDEX_NONE;
	bool bPole = false;
	if (!GetTarget(Asset, Track, bPole)) { return; }
	TSharedPtr<FScopedTransaction> Local;
	if (!SliderTransaction.IsValid()) { Local = MakeShared<FScopedTransaction>(LOCTEXT("EditControlStyleTx", "Edit Control Properties")); }
	FAnimStitchControlStyle Style = GetStyle();
	Mutator(Style);
	Asset->SetTrackControlStyle(Track, bPole, Style);
}

TSharedRef<SWidget> SAnimStitchControlProperties::MakeOffsetRow(const FText& Label, int32 Axis)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)[SNew(STextBlock).Text(Label)]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 1.f, 6.f, 1.f)
		[
			SNew(SSpinBox<float>).Delta(0.25f).MinFractionalDigits(1).MaxFractionalDigits(2)
			.ToolTipText(FText::Format(LOCTEXT("OffsetRowTip", "Moves only the drawn control along {0} (cm) so overlapping shapes can be told apart. The animation is not affected."), Label))
			.Value_Lambda([this, Axis]() { return float(GetStyle().Offset[Axis]); })
			.OnBeginSliderMovement_Lambda([this]() { SliderTransaction = MakeShared<FScopedTransaction>(LOCTEXT("DragControlStyleTx", "Edit Control Properties")); })
			.OnEndSliderMovement_Lambda([this](float) { SliderTransaction.Reset(); })
			.OnValueChanged_Lambda([this, Axis](float V) { Edit([&](FAnimStitchControlStyle& S) { S.Offset[Axis] = V; }); })
			.OnValueCommitted_Lambda([this, Axis](float V, ETextCommit::Type) { if (!SliderTransaction.IsValid() && !FMath::IsNearlyEqual(float(GetStyle().Offset[Axis]), V)) { Edit([&](FAnimStitchControlStyle& S) { S.Offset[Axis] = V; }); } })
		];
}

void SAnimStitchControlProperties::Construct(const FArguments& InArgs)
{
	Preview = InArgs._Preview;
	Timeline = InArgs._Timeline;

	auto HasTarget = [this]() { UAnimStitchTimelineAsset* A = nullptr; int32 T = INDEX_NONE; bool bP = false; return GetTarget(A, T, bP); };

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 8.f, 8.f, 4.f)
		[
			SNew(STextBlock).Font(FAppStyle::GetFontStyle("BoldFont")).AutoWrapText(true)
			.Text_Lambda([this, HasTarget]() -> FText
			{
				UAnimStitchTimelineAsset* Asset = nullptr; int32 Track = INDEX_NONE; bool bPole = false;
				if (!GetTarget(Asset, Track, bPole)) { return LOCTEXT("NoControlSelected", "Select a control shape in the viewport (or a bone track) to edit its look."); }
				return FText::Format(LOCTEXT("ControlTitle", "{0}{1}"), FText::FromName(Asset->Timeline.Tracks[Track].Bone), bPole ? LOCTEXT("PoleSuffix", " (pole)") : (Asset->Timeline.Tracks[Track].Kind == EAnimStitchTrackKind::IKLimb ? LOCTEXT("EffectorSuffix", " (IK)") : FText::GetEmpty()));
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 8.f, 8.f)
		[
			SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("ControlPropsHint", "Values you override here win over the global Show > Controls settings, for this control only."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 2.f)
			[
				SNew(SCheckBox)
				.IsEnabled_Lambda(HasTarget)
				.ToolTipText(LOCTEXT("ShowTrailTip", "Show this control's motion trail in the viewport (the G helper switch still hides every trail)"))
				.IsChecked_Lambda([this]() { UAnimStitchTimelineAsset* A = nullptr; int32 T = INDEX_NONE; bool bP = false; return GetTarget(A, T, bP) && !A->Timeline.Tracks[T].bShowTrail ? ECheckBoxState::Unchecked : ECheckBoxState::Checked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					UAnimStitchTimelineAsset* A = nullptr; int32 T = INDEX_NONE; bool bP = false;
					if (!GetTarget(A, T, bP)) { return; }
					const FScopedTransaction Tx(LOCTEXT("ShowTrailTx", "Show / Hide Motion Trail"));
					A->SetTrackShowTrail(T, State == ECheckBoxState::Checked);
				})
				[
					SNew(STextBlock).Text(LOCTEXT("ShowTrail", "Show motion trail"))
				]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 2.f)
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda(HasTarget)
			// Opacity
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SCheckBox)
					.ToolTipText(LOCTEXT("OverrideOpacityTip", "Override the global opacity for this control"))
					.IsChecked_Lambda([this]() { return GetStyle().bOverrideOpacity ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Edit([&](FAnimStitchControlStyle& S) { S.bOverrideOpacity = State == ECheckBoxState::Checked; if (S.bOverrideOpacity) { S.Opacity = Preview->GetControlOpacity(); } }); })
					[
						SNew(SBox).WidthOverride(64.f)[SNew(STextBlock).Text(LOCTEXT("PropOpacity", "Opacity"))]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpinBox<float>).MinValue(0.02f).MaxValue(1.f).Delta(0.02f)
					.ToolTipText(LOCTEXT("PropOpacityTip", "Opacity of this control's shape (needs the Opacity override ticked)"))
					.IsEnabled_Lambda([this]() { return GetStyle().bOverrideOpacity; })
					.Value_Lambda([this]() { const FAnimStitchControlStyle S = GetStyle(); return S.bOverrideOpacity ? S.Opacity : Preview->GetControlOpacity(); })
					.OnBeginSliderMovement_Lambda([this]() { SliderTransaction = MakeShared<FScopedTransaction>(LOCTEXT("DragControlStyleTx2", "Edit Control Properties")); })
					.OnEndSliderMovement_Lambda([this](float) { SliderTransaction.Reset(); })
					.OnValueChanged_Lambda([this](float V) { Edit([V](FAnimStitchControlStyle& S) { S.Opacity = V; }); })
					.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { if (!SliderTransaction.IsValid()) { Edit([V](FAnimStitchControlStyle& S) { S.Opacity = V; }); } })
				]
			]
			// Size
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SCheckBox)
					.ToolTipText(LOCTEXT("OverrideSizeTip", "Override the global Control Size for this control (a multiplier on the bone's own size)"))
					.IsChecked_Lambda([this]() { return GetStyle().bOverrideSize ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Edit([&](FAnimStitchControlStyle& S) { S.bOverrideSize = State == ECheckBoxState::Checked; if (S.bOverrideSize) { S.Size = Preview->GetControlDrawSize(); } }); })
					[
						SNew(SBox).WidthOverride(64.f)[SNew(STextBlock).Text(LOCTEXT("PropSize", "Size"))]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpinBox<float>).MinValue(0.1f).MaxValue(8.f).Delta(0.1f)
					.ToolTipText(LOCTEXT("PropSizeTip", "Size of this control's shape (needs the Size override ticked)"))
					.IsEnabled_Lambda([this]() { return GetStyle().bOverrideSize; })
					.Value_Lambda([this]() { const FAnimStitchControlStyle S = GetStyle(); return S.bOverrideSize ? S.Size : Preview->GetControlDrawSize(); })
					.OnBeginSliderMovement_Lambda([this]() { SliderTransaction = MakeShared<FScopedTransaction>(LOCTEXT("DragControlStyleTx3", "Edit Control Properties")); })
					.OnEndSliderMovement_Lambda([this](float) { SliderTransaction.Reset(); })
					.OnValueChanged_Lambda([this](float V) { Edit([V](FAnimStitchControlStyle& S) { S.Size = V; }); })
					.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { if (!SliderTransaction.IsValid()) { Edit([V](FAnimStitchControlStyle& S) { S.Size = V; }); } })
				]
			]
			// Shape
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[SNew(SBox).WidthOverride(84.f)[SNew(STextBlock).Text(LOCTEXT("PropShape", "Shape"))]]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
				[
					SNew(SCheckBox).Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.ToolTipText(LOCTEXT("ShapeBoxTip", "Draw this control as a box"))
					.IsChecked_Lambda([this]() { return GetStyle().Shape == EAnimStitchControlShape::Box ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState) { Edit([](FAnimStitchControlStyle& S) { S.Shape = EAnimStitchControlShape::Box; }); })
					[SNew(STextBlock).Text(LOCTEXT("ShapeBox", "Box")).Margin(FMargin(10.f, 2.f))]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox).Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.ToolTipText(LOCTEXT("ShapeSphereTip", "Draw this control as a sphere"))
					.IsChecked_Lambda([this]() { return GetStyle().Shape == EAnimStitchControlShape::Sphere ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState) { Edit([](FAnimStitchControlStyle& S) { S.Shape = EAnimStitchControlShape::Sphere; }); })
					[SNew(STextBlock).Text(LOCTEXT("ShapeSphere", "Sphere")).Margin(FMargin(10.f, 2.f))]
				]
			]
			// Offset
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)[SNew(STextBlock).Text(LOCTEXT("PropOffset", "Offset (cm), to separate overlapping shapes"))]
			+ SVerticalBox::Slot().AutoHeight()[MakeOffsetRow(LOCTEXT("OffX", "X"), 0)]
			+ SVerticalBox::Slot().AutoHeight()[MakeOffsetRow(LOCTEXT("OffY", "Y"), 1)]
			+ SVerticalBox::Slot().AutoHeight()[MakeOffsetRow(LOCTEXT("OffZ", "Z"), 2)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				SNew(SButton).Text(LOCTEXT("ResetControlStyle", "Reset to global"))
				.ToolTipText(LOCTEXT("ResetControlStyleTip", "Remove every override of this control"))
				.OnClicked_Lambda([this]() { Edit([](FAnimStitchControlStyle& S) { S = FAnimStitchControlStyle(); }); return FReply::Handled(); })
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
