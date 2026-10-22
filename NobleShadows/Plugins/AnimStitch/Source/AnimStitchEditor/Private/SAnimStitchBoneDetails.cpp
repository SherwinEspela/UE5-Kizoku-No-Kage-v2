#include "SAnimStitchBoneDetails.h"
#include "AnimStitchPreview.h"
#include "SAnimStitchTimeline.h"
#include "SAnimStitchKeyPanel.h"
#include "AnimStitchTimelineAsset.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

FText SAnimStitchBoneDetails::GetValue(ESpace Space, EPart Part, int32 Axis) const
{
	FTransform Local, Component;
	if (!Preview.IsValid() || !Preview->GetSelectedBoneTransforms(Local, Component))
	{
		return FText::GetEmpty();
	}
	const FTransform& T = Space == ESpace::Local ? Local : Component;
	double Value = 0.0;
	switch (Part)
	{
	case EPart::Translation:
		Value = T.GetTranslation()[Axis];
		break;
	case EPart::Rotation:
	{
		const FRotator R = T.Rotator(); // degrees: X = roll, Y = pitch, Z = yaw
		Value = Axis == 0 ? R.Roll : Axis == 1 ? R.Pitch : R.Yaw;
		break;
	}
	case EPart::Scale:
		Value = T.GetScale3D()[Axis];
		break;
	}
	return FText::FromString(FString::Printf(TEXT("%.3f"), Value));
}

FText SAnimStitchBoneDetails::GetHeader() const
{
	return Preview.IsValid() && Preview->GetSelectedBone() != INDEX_NONE ? FText::FromName(Preview->GetMeshBoneName(Preview->GetSelectedBone())) : FText::GetEmpty();
}

FText SAnimStitchBoneDetails::GetSubHeader() const
{
	if (!Preview.IsValid() || Preview->GetSelectedBone() == INDEX_NONE)
	{
		return FText::GetEmpty();
	}
	const int32 Bone = Preview->GetSelectedBone();
	const int32 Parent = Preview->GetMeshBoneParent(Bone);
	return FText::Format(LOCTEXT("BoneSubHeader", "Bone {0}  |  parent: {1}"), Bone, Parent == INDEX_NONE ? LOCTEXT("NoParent", "(none)") : FText::FromName(Preview->GetMeshBoneName(Parent)));
}

bool SAnimStitchBoneDetails::GetSelectedControlTrack(int32& OutTrack) const
{
	if (!Preview.IsValid() || !Timeline.IsValid() || !Timeline->GetAsset()) { return false; }
	const FAnimStitchControlId Id = Preview->GetSelectedControl();
	const bool bTrackControl = Id.Kind == EAnimStitchControlKind::Bone || Id.Kind == EAnimStitchControlKind::IKEffector || Id.Kind == EAnimStitchControlKind::IKPole;
	OutTrack = Id.A;
	return bTrackControl && Timeline->GetAsset()->Timeline.Tracks.IsValidIndex(Id.A);
}

EVisibility SAnimStitchBoneDetails::GetControlVisibility() const
{
	int32 Track = INDEX_NONE;
	return GetSelectedControlTrack(Track) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SAnimStitchBoneDetails::GetControlTitle() const
{
	int32 Track = INDEX_NONE;
	if (!GetSelectedControlTrack(Track)) { return FText::GetEmpty(); }
	const FAnimStitchTrack& T = Timeline->GetAsset()->Timeline.Tracks[Track];
	const FAnimStitchControlId Id = Preview->GetSelectedControl();
	return FText::Format(LOCTEXT("ControlDetailsTitle", "Control: {0}{1}"), FText::FromName(T.Bone), Id.Kind == EAnimStitchControlKind::IKPole ? LOCTEXT("CtrlPole", " (pole)") : (T.Kind == EAnimStitchTrackKind::IKLimb ? LOCTEXT("CtrlIK", " (IK)") : FText::GetEmpty()));
}

EVisibility SAnimStitchBoneDetails::GetNoSelectionVisibility() const
{
	int32 Track = INDEX_NONE;
	return (Preview.IsValid() && Preview->GetSelectedBone() != INDEX_NONE) || GetSelectedControlTrack(Track) ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SAnimStitchBoneDetails::GetSelectionVisibility() const
{
	return Preview.IsValid() && Preview->GetSelectedBone() != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SAnimStitchBoneDetails::MakeGroup(const FText& Title, ESpace Space)
{
	auto Row = [this, Space](const FText& Label, EPart Part)
	{
		TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
			[
				SNew(SBox).WidthOverride(70.f)[SNew(STextBlock).Text(Label)]
			];
		static const TCHAR* Axes[3] = { TEXT("X"), TEXT("Y"), TEXT("Z") };
		static const TCHAR* RotAxes[3] = { TEXT("R"), TEXT("P"), TEXT("Y") }; // roll, pitch, yaw
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			Box->AddSlot().FillWidth(1.f).Padding(0, 0, 4, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 3, 0)
				[
					SNew(STextBlock).Text(FText::FromString(Part == EPart::Rotation ? RotAxes[Axis] : Axes[Axis])).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SEditableTextBox).IsReadOnly(true).ToolTipText(LOCTEXT("BoneValueTip", "Read-only: this bone's transform as shown in the preview, including foot lock and manual corrections. Edit it with a bone track.")).Text_Lambda([this, Space, Part, Axis]() { return GetValue(Space, Part, Axis); })
				]
			];
		}
		return Box;
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 4)
		[
			SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)[Row(LOCTEXT("Translation", "Translation"), EPart::Translation)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)[Row(LOCTEXT("Rotation", "Rotation"), EPart::Rotation)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)[Row(LOCTEXT("Scale", "Scale"), EPart::Scale)];
}

void SAnimStitchBoneDetails::Construct(const FArguments& InArgs)
{
	Preview = InArgs._Preview;
	Timeline = InArgs._Timeline;
	const TFunction<bool(int32, FAnimStitchTrackKey&)> LiveIK = InArgs._LiveIK;

	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("DetailsTitle", "Details")).Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			// The selected control: its transform and, for an IK limb, the weight. Typing writes / adjusts the key at the playhead, like dragging.
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SVerticalBox).Visibility(this, &SAnimStitchBoneDetails::GetControlVisibility)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(this, &SAnimStitchBoneDetails::GetControlTitle).Font(FAppStyle::GetFontStyle("BoldFont")).ColorAndOpacity(FLinearColor(0.5f, 0.85f, 1.f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
				[
					SNew(SAnimStitchKeyPanel).Timeline(Timeline)
					.TrackIndex_Lambda([this]() { int32 T = INDEX_NONE; GetSelectedControlTrack(T); return T; })
					.LiveIK(LiveIK)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(STextBlock).Visibility(this, &SAnimStitchBoneDetails::GetNoSelectionVisibility).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(LOCTEXT("DetailsNoSelection", "No bone selected. Turn the skeleton on in the viewport (top right) and click a joint, or pick a bone from the dropdown."))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox).Visibility(this, &SAnimStitchBoneDetails::GetSelectionVisibility)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
					[
						SNew(STextBlock).Text(this, &SAnimStitchBoneDetails::GetHeader).Font(FAppStyle::GetFontStyle("BoldFont")).ColorAndOpacity(FLinearColor(1.f, 0.9f, 0.3f))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(this, &SAnimStitchBoneDetails::GetSubHeader).ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot().AutoHeight()[MakeGroup(LOCTEXT("ComponentGroup", "Component (mesh space, includes root motion)"), ESpace::Component)]
					+ SVerticalBox::Slot().AutoHeight()[MakeGroup(LOCTEXT("LocalGroup", "Local (relative to parent)"), ESpace::Local)]
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE


