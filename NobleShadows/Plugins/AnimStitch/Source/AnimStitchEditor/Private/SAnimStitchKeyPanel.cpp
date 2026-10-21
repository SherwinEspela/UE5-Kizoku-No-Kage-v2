#include "SAnimStitchKeyPanel.h"
#include "SAnimStitchTimeline.h"
#include "AnimStitchManualEdits.h"
#include "AnimStitchTimelineAsset.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
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
	const int32 Index = TrackIndexAttr.Get(INDEX_NONE);
	return Asset && Asset->Timeline.Tracks.IsValidIndex(Index) ? &Asset->Timeline.Tracks[Index] : nullptr;
}

FAnimStitchTrackKey SAnimStitchKeyPanel::GetKey() const
{
	FAnimStitchTrackKey Key;
	const FAnimStitchTrack* Track = GetTrack();
	if (!Track) { return Key; }
	const int32 KeyIndex = Timeline->GetSelectedKeyFor(TrackIndexAttr.Get(INDEX_NONE));
	if (Track->Kind == EAnimStitchTrackKind::Bone)
	{
		// Every channel shows its own interpolated value at the selected key's time (or the playhead): a key that holds only some channels still shows the rest.
		const double Time = Track->Keys.IsValidIndex(KeyIndex) ? Track->Keys[KeyIndex].Time : Timeline->GetPlayhead();
		Key = FAnimStitchManualEdits::Sample(*Track, Time);
		Key.Time = Time;
	}
	else if (Track->Keys.IsValidIndex(KeyIndex))
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
	if (Track->Kind == EAnimStitchTrackKind::Bone && Track->Mode == EAnimStitchTrackMode::Absolute && !Track->Keys.IsValidIndex(KeyIndex) && LiveIK)
	{
		// Absolute: with no key selected the panel shows (and starts editing from) the bone's full transform as it is evaluated on screen.
		FAnimStitchTrackKey Live;
		if (LiveIK(TrackIndexAttr.Get(INDEX_NONE), Live))
		{
			Key.Translation = Live.Translation;
			Key.Rotation = Live.Rotation;
			Key.Scale = Live.Scale;
		}
	}
	if (Track->Kind == EAnimStitchTrackKind::IKLimb && Key.Weight < 1e-3f && LiveIK)
	{
		// The key does not hold a target (IK off here): show and start from the limb as it is on screen.
		FAnimStitchTrackKey Live;
		if (LiveIK(TrackIndexAttr.Get(INDEX_NONE), Live))
		{
			Key.Translation = Live.Translation;
			Key.Rotation = Live.Rotation;
			Key.Pole = Live.Pole;
		}
	}
	return Key;
}

bool SAnimStitchKeyPanel::IsBoneTrack() const
{
	const FAnimStitchTrack* Track = GetTrack();
	return Track && Track->Kind == EAnimStitchTrackKind::Bone;
}

bool SAnimStitchKeyPanel::IsChannelLocked(uint8 Channel) const
{
	const FAnimStitchTrack* Track = GetTrack();
	return Track && (FAnimStitchManualEdits::GetEditableChannels(*Track) & Channel) == 0;
}

void SAnimStitchKeyPanel::Edit(TFunctionRef<void(FAnimStitchTrackKey&)> Mutator, uint8 Channel)
{
	UAnimStitchTimelineAsset* Asset = Timeline->GetAsset();
	const int32 TrackIndex = TrackIndexAttr.Get(INDEX_NONE);
	if (!Asset || TrackIndex == INDEX_NONE) { return; }
	if (IsBoneTrack() && IsChannelLocked(Channel)) { return; } // a locked channel is read-only
	TSharedPtr<FScopedTransaction> Local;
	if (!SliderTransaction.IsValid())
	{
		Local = MakeShared<FScopedTransaction>(LOCTEXT("EditKeyValueTx", "Edit Key Value"));
	}
	FAnimStitchTrackKey Key = GetKey();
	Mutator(Key);
	if (IsBoneTrack()) { Asset->SetTrackChannelKey(TrackIndex, Key, Channel); } // only the edited channel is keyed
	else { Asset->SetTrackKey(TrackIndex, Key); }
}

TSharedRef<SWidget> SAnimStitchKeyPanel::Vec3Row(const FText& Label, TFunction<FVector(const FAnimStitchTrackKey&)> Get, TFunction<void(FAnimStitchTrackKey&, const FVector&)> Set, TAttribute<EVisibility> Visible, double Delta, uint8 Channel)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	// Channel rows of a bone track start with the lane's lock toggle; a locked channel keeps its keys visible but is read-only here.
	Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
	[
		SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)
		.Visibility_Lambda([this, Channel]() { return Channel != 0 && IsBoneTrack() ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(2.f))
			.ToolTipText_Lambda([this, Channel]() { return IsChannelLocked(Channel) ? LOCTEXT("ChannelLockedTip", "Locked: the keys on this channel lane cannot be moved or edited. Click to unlock.") : LOCTEXT("ChannelUnlockedTip", "Unlocked: this channel can be keyed and edited. Click to lock."); })
			.OnClicked_Lambda([this, Channel]()
			{
				UAnimStitchTimelineAsset* Asset = Timeline->GetAsset();
				const FAnimStitchTrack* Track = GetTrack();
				if (Asset && Track)
				{
					const FScopedTransaction Tx(LOCTEXT("ToggleChannelLockTx", "Lock / Unlock Channel"));
					Asset->SetTrackChannelUnlocked(TrackIndexAttr.Get(INDEX_NONE), Channel, (Track->UnlockedChannels & Channel) == 0);
				}
				return FReply::Handled();
			})
			[
				SNew(SImage).Image_Lambda([this, Channel]() { return FAppStyle::GetBrush(IsChannelLocked(Channel) ? "Icons.Lock" : "Icons.Unlock"); })
			]
		]
	];
	Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 3.f, 0.f)
	[
		SNew(SBox).WidthOverride_Lambda([this, Channel]() -> FOptionalSize { return FOptionalSize(Channel != 0 && IsBoneTrack() ? 40.f : 62.f); })[SNew(STextBlock).Text(Label)]
	];
	static const TCHAR* AxisNames[3] = { TEXT("X"), TEXT("Y"), TEXT("Z") };
	static const FLinearColor AxisColors[3] = { FLinearColor(0.9f, 0.25f, 0.25f), FLinearColor(0.3f, 0.8f, 0.3f), FLinearColor(0.3f, 0.5f, 1.f) };
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)[SNew(STextBlock).Text(FText::FromString(AxisNames[Axis])).ColorAndOpacity(AxisColors[Axis])];
		Row->AddSlot().AutoWidth().Padding(0.f, 1.f, 3.f, 1.f)
		[
			SNew(SBox).WidthOverride(54.f)
			[
				SNew(SSpinBox<double>)
				.IsEnabled_Lambda([this, Channel]() { return !(Channel != 0 && IsBoneTrack() && IsChannelLocked(Channel)); })
				.Delta(Delta).MinFractionalDigits(1).MaxFractionalDigits(2)
				.Value_Lambda([this, Get, Axis]() { return Get(GetKey())[Axis]; })
				.OnBeginSliderMovement_Lambda([this]() { SliderTransaction = MakeShared<FScopedTransaction>(LOCTEXT("DragKeyValueTx", "Edit Key Value")); })
				.OnEndSliderMovement_Lambda([this](double) { SliderTransaction.Reset(); })
				.OnValueChanged_Lambda([this, Get, Set, Axis, Channel](double V)
				{
					Edit([&](FAnimStitchTrackKey& Key) { FVector Vec = Get(Key); if (FMath::IsNearlyEqual(Vec[Axis], V, 1e-9)) { return; } Vec[Axis] = V; Set(Key, Vec); }, Channel ? Channel : AnimStitchChannel::All);
				})
				.OnValueCommitted_Lambda([this, Get, Set, Axis, Channel](double V, ETextCommit::Type)
				{
					if (SliderTransaction.IsValid()) { return; }
					if (FMath::IsNearlyEqual(Get(GetKey())[Axis], V, 1e-6)) { return; }
					Edit([&](FAnimStitchTrackKey& Key) { FVector Vec = Get(Key); Vec[Axis] = V; Set(Key, Vec); }, Channel ? Channel : AnimStitchChannel::All);
				})
			]
		];
	}
	// A fixed-height slot: the rows line up with the channel lanes drawn beside them in the timeline.
	return SNew(SBox).HeightOverride((SAnimStitchTimeline::ExpanderHeight - 2.f) / 4.f).VAlign(VAlign_Center).Visibility(Visible)[Row];
}

void SAnimStitchKeyPanel::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;
	LiveIK = InArgs._LiveIK;
	TrackIndexAttr = InArgs._TrackIndex;

	auto IsIK = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::IKLimb; };
	auto IsBone = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::Bone && FAnimStitchManualEdits::IsPelvisBone(T->Bone); }; // Keep feet planted: pelvis only
	auto ShowTranslate = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && !(T->Kind == EAnimStitchTrackKind::Bone && T->bRotationOnly); };
	auto IsPoleLane = [this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::IKPole; };

	ChildSlot
	[
		SNew(SBox).HeightOverride(SAnimStitchTimeline::ExpanderHeight - 2.f)
		[
			SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowTranslate", "Translate"),
					[](const FAnimStitchTrackKey& K) { return K.Translation; },
					[this](FAnimStitchTrackKey& K, const FVector& V) { K.Translation = V; if (GetTrack() && GetTrack()->Kind == EAnimStitchTrackKind::IKLimb && K.Weight < 1e-3f) { K.Weight = 1.f; } },
					TAttribute<EVisibility>::CreateLambda([this, ShowTranslate]() { return ShowTranslate() ? EVisibility::Visible : (IsBoneTrack() ? EVisibility::Hidden : EVisibility::Collapsed); }), 0.5, AnimStitchChannel::Translation)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowRotate", "Rotate"),
					[](const FAnimStitchTrackKey& K) { return ToEuler(K.Rotation); },
					[this](FAnimStitchTrackKey& K, const FVector& V) { K.Rotation = FromEuler(V); if (GetTrack() && GetTrack()->Kind == EAnimStitchTrackKind::RootOffset) { K.Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(K.Rotation.Rotator().Yaw)); } if (GetTrack() && GetTrack()->Kind == EAnimStitchTrackKind::IKLimb && K.Weight < 1e-3f) { K.Weight = 1.f; } },
					TAttribute<EVisibility>::CreateLambda([IsPoleLane]() { return IsPoleLane() ? EVisibility::Collapsed : EVisibility::Visible; }), 0.5, AnimStitchChannel::Rotation)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Vec3Row(LOCTEXT("RowScale", "Scale"),
					[](const FAnimStitchTrackKey& K) { return K.Scale; },
					[](FAnimStitchTrackKey& K, const FVector& V) { K.Scale = V; },
					TAttribute<EVisibility>::CreateLambda([this]() { const FAnimStitchTrack* T = GetTrack(); return T && T->Kind == EAnimStitchTrackKind::Bone ? (T->bRotationOnly ? EVisibility::Hidden : EVisibility::Visible) : EVisibility::Collapsed; }), 0.01, AnimStitchChannel::Scale)
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
							Asset->SetTrackKeepFeet(TrackIndexAttr.Get(INDEX_NONE), State == ECheckBoxState::Checked);
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
