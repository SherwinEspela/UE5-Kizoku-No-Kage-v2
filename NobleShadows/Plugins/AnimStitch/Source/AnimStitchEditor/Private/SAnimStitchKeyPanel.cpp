#include "SAnimStitchKeyPanel.h"
#include "SAnimStitchTimeline.h"
#include "AnimStitchManualEdits.h"
#include "AnimStitchTimelineAsset.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace
{
	FVector ToEuler(const FQuat& Q)
	{
		const FRotator R = Q.Rotator();
		return FVector(R.Roll, R.Pitch, R.Yaw);
	}

	FQuat FromEuler(const FVector& V)
	{
		return FRotator(V.Y, V.Z, V.X).Quaternion();
	}
}

const FAnimStitchTrack* SAnimStitchKeyPanel::GetTrack() const
{
	const UAnimStitchTimelineAsset* Asset = Timeline->GetAsset();
	const int32 Index = Timeline->GetExpandedTrack();
	return Asset && Asset->Timeline.Tracks.IsValidIndex(Index) ? &Asset->Timeline.Tracks[Index] : nullptr;
}

FAnimStitchTrackKey SAnimStitchKeyPanel::GetKey() const
{
	FAnimStitchTrackKey Key;
	const FAnimStitchTrack* Track = GetTrack();
	if (!Track) { return Key; }
	const int32 KeyIndex = Timeline->GetExpandedKey();
	if (Track->Keys.IsValidIndex(KeyIndex))
	{
		Key = Track->Keys[KeyIndex];
	}
	else if (Track->Keys.Num() > 0)
	{
		Key = FAnimStitchManualEdits::Sample(*Track, Timeline->GetPlayhead());
		Key.Time = Timeline->GetPlayhead();
	}
	else
	{
		Key.Time = Timeline->GetPlayhead();
	}
	if (Track->Kind == EAnimStitchTrackKind::IKLimb && Key.Weight < 1e-3f && LiveIK)
	{
		// The key does not hold a target (IK off here): show and start from the limb as it is on screen.
		FAnimStitchTrackKey Live;
		if (LiveIK(Timeline->GetExpandedTrack(), Live))
		{
			Key.Translation = Live.Translation;
			Key.Rotation = Live.Rotation;
			Key.Pole = Live.Pole;
		}
	}
	return Key;
}

void SAnimStitchKeyPanel::Edit(TFunctionRef<void(FAnimStitchTrackKey&)> Mutator)
{
	UAnimStitchTimelineAsset* Asset = Timeline->GetAsset();
	const int32 TrackIndex = Timeline->GetExpandedTrack();
	if (!Asset || TrackIndex == INDEX_NONE) { return; }
	TSharedPtr<FScopedTransaction> Local;
	if (!SliderTransaction.IsValid())
	{
		Local = MakeShared<FScopedTransaction>(LOCTEXT("EditKeyValueTx", "Edit Key Value"));
	}
	FAnimStitchTrackKey Key = GetKey();
	Mutator(Key);
	Asset->SetTrackKey(TrackIndex, Key);
}

TSharedRef<SWidget> SAnimStitchKeyPanel::Vec3Row(const FText& Label, TFunction<FVector(const FAnimStitchTrackKey&)> Get, TFunction<void(FAnimStitchTrackKey&, const FVector&)> Set, TAttribute<EVisibility> Visible, double Delta)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox).Visibility(Visible);
	Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 4.f, 0.f)
	[
		SNew(SBox).WidthOverride(54.f)[SNew(STextBlock).Text(Label)]
	];
	static const TCHAR* AxisNames[3] = { TEXT("X"), TEXT("Y"), TEXT("Z") };
	static const FLinearColor AxisColors[3] = { FLinearColor(0.9f, 0.25f, 0.25f), FLinearColor(0.3f, 0.8f, 0.3f), FLinearColor(0.3f, 0.5f, 1.f) };
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)[SNew(STextBlock).Text(FText::FromString(AxisNames[Axis])).ColorAndOpacity(AxisColors[Axis])];
		Row->AddSlot().AutoWidth().Padding(0.f, 1.f, 3.f, 1.f)
		[
			SNew(SBox).WidthOverride(58.f)
			[
				SNew(SSpinBox<double>)
				.Delta(Delta).MinFractionalDigits(1).MaxFractionalDigits(2)
				.Value_Lambda([this, Get, Axis]() { return Get(GetKey())[Axis]; })
				.OnBeginSliderMovement_Lambda([this]() { SliderTransaction = MakeShared<FScopedTransaction>(LOCTEXT("DragKeyValueTx", "Edit Key Value")); })
				.OnEndSliderMovement_Lambda([this](double) { SliderTransaction.Reset(); })
				.OnValueChanged_Lambda([this, Get, Set, Axis](double V)
				{
					Edit([&](FAnimStitchTrackKey& Key) { FVector Vec = Get(Key); if (FMath::IsNearlyEqual(Vec[Axis], V, 1e-9)) { return; } Vec[Axis] = V; Set(Key, Vec); });
				})
				.OnValueCommitted_Lambda([this, Get, Set, Axis](double V, ETextCommit::Type)
				{
					if (SliderTransaction.IsValid()) { return; }
					if (FMath::IsNearlyEqual(Get(GetKey())[Axis], V, 1e-6)) { return; }
					Edit([&](FAnimStitchTrackKey& Key) { FVector Vec = Get(Key); Vec[Axis] = V; Set(Key, Vec); });
				})
			]
		];
	}
	return Row;
}

void SAnimStitchKeyPanel::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;
	LiveIK = InArgs._LiveIK;

	auto IsIK = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::IKLimb; };
	auto IsBone = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::Bone && FAnimStitchManualEdits::IsPelvisBone(T->Bone); }; // Keep feet planted: pelvis only
	auto ShowTranslate = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && !(T->Kind == EAnimStitchTrackKind::Bone && T->bRotationOnly); };

	ChildSlot
	[
		SNew(SBox).HeightOverride(SAnimStitchTimeline::ExpanderHeight - 2.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)[SNullWidget::NullWidget]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowTranslate", "Translate"),
					[](const FAnimStitchTrackKey& K) { return K.Translation; },
					[this](FAnimStitchTrackKey& K, const FVector& V) { K.Translation = V; if (GetTrack() && GetTrack()->Kind == EAnimStitchTrackKind::IKLimb && K.Weight < 1e-3f) { K.Weight = 1.f; } },
					TAttribute<EVisibility>::CreateLambda([ShowTranslate]() { return ShowTranslate() ? EVisibility::Visible : EVisibility::Collapsed; }), 0.5)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowRotate", "Rotate"),
					[](const FAnimStitchTrackKey& K) { return ToEuler(K.Rotation); },
					[this](FAnimStitchTrackKey& K, const FVector& V) { K.Rotation = FromEuler(V); if (GetTrack() && GetTrack()->Kind == EAnimStitchTrackKind::IKLimb && K.Weight < 1e-3f) { K.Weight = 1.f; } },
					EVisibility::Visible, 0.5)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowScale", "Scale"),
					[](const FAnimStitchTrackKey& K) { return K.Scale; },
					[](FAnimStitchTrackKey& K, const FVector& V) { K.Scale = V; },
					TAttribute<EVisibility>::CreateLambda([this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::Bone && !T->bRotationOnly ? EVisibility::Visible : EVisibility::Collapsed; }), 0.01)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowPole", "Pole target"),
					[](const FAnimStitchTrackKey& K) { return K.Pole; },
					[this](FAnimStitchTrackKey& K, const FVector& V) { K.Pole = V; if (K.Weight < 1e-3f) { K.Weight = 1.f; } },
					TAttribute<EVisibility>::CreateLambda([IsIK]() { return IsIK() ? EVisibility::Visible : EVisibility::Collapsed; }), 0.5)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SBox).WidthOverride(54.f)
					[
						SNew(STextBlock).Text(LOCTEXT("RowWeight", "IK weight"))
						.Visibility_Lambda([IsIK]() { return IsIK() ? EVisibility::Visible : EVisibility::Collapsed; })
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(70.f)
					.Visibility_Lambda([IsIK]() { return IsIK() ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SSpinBox<float>).MinValue(0.f).MaxValue(1.f).Delta(0.05f)
						.Value_Lambda([this]() { return GetKey().Weight; })
						.OnBeginSliderMovement_Lambda([this]() { SliderTransaction = MakeShared<FScopedTransaction>(LOCTEXT("DragKeyValueTx", "Edit Key Value")); })
						.OnEndSliderMovement_Lambda([this](float) { SliderTransaction.Reset(); })
						.OnValueChanged_Lambda([this](float V) { Edit([V](FAnimStitchTrackKey& K) { K.Weight = V; }); })
						.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { if (!SliderTransaction.IsValid() && !FMath::IsNearlyEqual(GetKey().Weight, V, 1e-6f)) { Edit([V](FAnimStitchTrackKey& K) { K.Weight = V; }); } })
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Visibility_Lambda([IsBone]() { return IsBone() ? EVisibility::Visible : EVisibility::Collapsed; })
					.ToolTipText(LOCTEXT("KeepFeetTip", "When this bone moves the legs (pelvis...), the feet stay where the animation had them, solved with leg IK. A foot with its own active IK track is left to that track."))
					.IsChecked_Lambda([this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->bKeepFeetPlanted ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						if (UAnimStitchTimelineAsset* Asset = Timeline->GetAsset())
						{
							const FScopedTransaction Tx(LOCTEXT("KeepFeetTx", "Keep Feet Planted"));
							Asset->SetTrackKeepFeet(Timeline->GetExpandedTrack(), State == ECheckBoxState::Checked);
						}
					})
					[
						SNew(STextBlock).Text(LOCTEXT("KeepFeet", "Keep feet planted (leg IK)"))
					]
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
