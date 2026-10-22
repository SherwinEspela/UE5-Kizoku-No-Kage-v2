#include "SAnimStitchFootLockSettings.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchFootLock.h"
#include "AnimStitchSettings.h"
#include "ScopedTransaction.h"
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchableComboBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#include "AnimStitchHelp.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchFootLockSettings::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Timeline);
}

const FAnimStitchFootLockSettings& SAnimStitchFootLockSettings::Current() const
{
	return Timeline->Timeline.FootLock;
}

void SAnimStitchFootLockSettings::Apply(TFunctionRef<void(FAnimStitchFootLockSettings&)> Edit, const FText& Label)
{
	FAnimStitchFootLockSettings Settings = Current();
	Edit(Settings);
	// Make the bones that are shown explicit, so what you see is what is used.
	if (Settings.LeftFootBone.IsNone() && LeftSelected.IsValid()) { Settings.LeftFootBone = FName(**LeftSelected); }
	if (Settings.RightFootBone.IsNone() && RightSelected.IsValid()) { Settings.RightFootBone = FName(**RightSelected); }
	{
		const FScopedTransaction Tx(Label);
		Timeline->SetFootLockSettings(Settings);
	}

	// Remember the foot bones for this skeleton.
	if (const USkeleton* Skeleton = Timeline->GetSkeleton())
	{
		if (!Settings.LeftFootBone.IsNone() && !Settings.RightFootBone.IsNone())
		{
			UAnimStitchSettings* Plugin = GetMutableDefault<UAnimStitchSettings>();
			FAnimStitchFootBones& Remembered = Plugin->FootBonesBySkeleton.FindOrAdd(Skeleton->GetPathName());
			if (Remembered.Left != Settings.LeftFootBone || Remembered.Right != Settings.RightFootBone)
			{
				Remembered.Left = Settings.LeftFootBone;
				Remembered.Right = Settings.RightFootBone;
				Plugin->SaveConfig();
			}
		}
	}
}

void SAnimStitchFootLockSettings::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;

	if (const USkeleton* Skeleton = Timeline->GetSkeleton())
	{
		const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
		for (int32 b = 0; b < Ref.GetNum(); ++b)
		{
			BoneItems.Add(MakeShared<FString>(Ref.GetBoneName(b).ToString()));
		}
		FName Left, Right;
		FAnimStitchFootLock::ResolveFootBones(Skeleton, Current(), Left, Right);
		for (const TSharedPtr<FString>& Item : BoneItems)
		{
			if (*Item == Left.ToString()) { LeftSelected = Item; }
			if (*Item == Right.ToString()) { RightSelected = Item; }
		}
	}

	auto Spin = [this](float Min, float Max, float Width, TFunction<float(const FAnimStitchFootLockSettings&)> Get, TFunction<void(FAnimStitchFootLockSettings&, float)> Set, const FText& Tip)
	{
		return SNew(SBox).WidthOverride(Width)
		[
			SNew(SSpinBox<float>).MinValue(Min).MaxValue(Max).ToolTipText(Tip)
			.Value_Lambda([this, Get]() { return Get(Current()); })
			.OnValueCommitted_Lambda([this, Set](float V, ETextCommit::Type) { Apply([&](FAnimStitchFootLockSettings& S) { Set(S, V); }, LOCTEXT("FootLockEditTx", "Change Foot Lock Settings")); })
			.OnEndSliderMovement_Lambda([this, Set](float V) { Apply([&](FAnimStitchFootLockSettings& S) { Set(S, V); }, LOCTEXT("FootLockEditTx", "Change Foot Lock Settings")); })
		];
	};

	auto BoneCombo = [this](TSharedPtr<FString>& Selected, bool bLeft)
	{
		return SNew(SSearchableComboBox)
			.ToolTipText(bLeft ? LOCTEXT("LeftFootBoneTip", "The left foot bone used for plant detection and pinning. Remembered per skeleton.") : LOCTEXT("RightFootBoneTip", "The right foot bone used for plant detection and pinning. Remembered per skeleton."))
			.OptionsSource(&BoneItems)
			.InitiallySelectedItem(Selected)
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(*Item)); })
			.OnSelectionChanged_Lambda([this, &Selected, bLeft](TSharedPtr<FString> Item, ESelectInfo::Type)
			{
				if (!Item.IsValid()) { return; }
				Selected = Item;
				Apply([&](FAnimStitchFootLockSettings& S) { (bLeft ? S.LeftFootBone : S.RightFootBone) = FName(**Item); }, LOCTEXT("FootBoneTx", "Change Foot Bone"));
			})
			[
				SNew(STextBlock).Text_Lambda([&Selected]() { return Selected.IsValid() ? FText::FromString(*Selected) : LOCTEXT("PickBone", "Pick a bone..."); })
			];
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(STextBlock).Text(LOCTEXT("FootLockHeader", "Foot lock settings")).Font(FAppStyle::GetFontStyle("BoldFont"))]
			+ SHorizontalBox::Slot().AutoWidth()[FAnimStitchHelp::MakeHelpButton(EAnimStitchHelpPage::FootLock, LOCTEXT("FootLockHelpTip", "Help: opens the Foot Lock page of the online documentation in your browser"))]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("LeftFoot", "Left foot"))]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 10, 0)[BoneCombo(LeftSelected, true)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("RightFoot", "Right foot"))]
			+ SHorizontalBox::Slot().FillWidth(1.f)[BoneCombo(RightSelected, false)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return Current().bAutoGroundHeight ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Apply([&](FAnimStitchFootLockSettings& S) { S.bAutoGroundHeight = State == ECheckBoxState::Checked; }, LOCTEXT("GroundModeTx", "Change Ground Height Mode")); })
				.ToolTipText(LOCTEXT("AutoGroundTip", "Ground height = the height of the foot bones in the skeleton's reference pose"))
				[
					SNew(STextBlock).Text(LOCTEXT("AutoGround", "Ground height from feet at rest"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("GroundHeight", "Ground Z (cm)"))]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).IsEnabled_Lambda([this]() { return !Current().bAutoGroundHeight; })
				[
					Spin(-10000.f, 10000.f, 80.f, [](const FAnimStitchFootLockSettings& S) { return S.GroundHeight; }, [](FAnimStitchFootLockSettings& S, float V) { S.GroundHeight = V; },
						LOCTEXT("GroundTip", "Ground-plane height (cm) used when it is not taken from the rest pose"))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return Current().bPelvisFollows ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Apply([&](FAnimStitchFootLockSettings& S) { S.bPelvisFollows = State == ECheckBoxState::Checked; }, LOCTEXT("PelvisTx", "Change Pelvis Follow")); })
				.ToolTipText(LOCTEXT("PelvisTip", "Move the pelvis (spine and head follow) just enough that locked legs are not stretched straight when the body moves away from a locked foot"))
				[
					SNew(STextBlock).Text(LOCTEXT("PelvisFollows", "Pelvis follows locked feet"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("MaxPelvis", "Max offset (cm)"))]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).IsEnabled_Lambda([this]() { return Current().bPelvisFollows; })
				[
					Spin(0.f, 200.f, 62.f, [](const FAnimStitchFootLockSettings& S) { return S.MaxPelvisOffset; }, [](FAnimStitchFootLockSettings& S, float V) { S.MaxPelvisOffset = V; },
						LOCTEXT("MaxPelvisTip", "The pelvis never moves farther than this from the animation. Beyond it the leg extends fully, so add an unlock."))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)[SNew(STextBlock).Text(LOCTEXT("PlantHeight", "Height <"))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)[Spin(0.f, 100.f, 62.f, [](const FAnimStitchFootLockSettings& S) { return S.HeightThreshold; }, [](FAnimStitchFootLockSettings& S, float V) { S.HeightThreshold = V; },
				LOCTEXT("PlantHeightTip", "Max height (cm) of the foot bone above the ground to count as planted"))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)[SNew(STextBlock).Text(LOCTEXT("PlantSpeed", "Speed <"))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)[Spin(0.f, 1000.f, 62.f, [](const FAnimStitchFootLockSettings& S) { return S.VelocityThreshold; }, [](FAnimStitchFootLockSettings& S, float V) { S.VelocityThreshold = V; },
				LOCTEXT("PlantSpeedTip", "Max world speed (cm/s, root motion included) to count as planted"))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)[SNew(STextBlock).Text(LOCTEXT("PlantMin", "Min plant (s)"))]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)[Spin(0.f, 2.f, 56.f, [](const FAnimStitchFootLockSettings& S) { return S.MinPlantDuration; }, [](FAnimStitchFootLockSettings& S, float V) { S.MinPlantDuration = V; },
				LOCTEXT("PlantMinTip", "Detected plants shorter than this are ignored"))]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)[SNew(STextBlock).Text(LOCTEXT("PlantBlend", "Blend (s)"))]
			+ SHorizontalBox::Slot().AutoWidth()[Spin(0.f, 1.f, 56.f, [](const FAnimStitchFootLockSettings& S) { return S.BlendTime; }, [](FAnimStitchFootLockSettings& S, float V) { S.BlendTime = V; },
				LOCTEXT("PlantBlendTip", "The correction fades in and out over this long at the ends of a plant"))]
		]
	];
}

#undef LOCTEXT_NAMESPACE
