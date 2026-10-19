// Manual bone correction lanes of the timeline widget (Phase 9): painting, "Add Bone Track" and lane menus.

#include "SAnimStitchTimeline.h"
#include "AnimStitchManualEdits.h"
#include "AnimStitchFootLock.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchableComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchTimeline::PaintTrackLanes(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G) const
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	if (TL.Tracks.Num() == 0)
	{
		return;
	}
	const FVector2f Size = G.GetLocalSize();
	CachedWidth = Size.X;
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	const FSlateBrush* Circle = FAppStyle::GetBrush("Icons.FilledCircle");
	const float AreaTop = GetTracksTop();
	Out.PushClip(FSlateClippingZone(G.MakeChild(FVector2f(Size.X, FMath::Max(1.f, Size.Y - AreaTop)), FSlateLayoutTransform(FVector2f(0.f, AreaTop)))));

	for (int32 i = 0; i < TL.Tracks.Num(); ++i)
	{
		const FAnimStitchTrack& Track = TL.Tracks[i];
		const float Top = GetTrackLaneTop(i);
		const bool bIK = Track.Kind == EAnimStitchTrackKind::IKLimb;
		const FLinearColor Base = bIK ? FLinearColor(0.95f, 0.5f, 0.15f) : (Track.bRotationOnly ? FLinearColor(0.9f, 0.6f, 0.2f) : FLinearColor(0.35f, 0.85f, 0.5f));

		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size.X - LabelWidth, TrackLaneHeight), FSlateLayoutTransform(FVector2f(LabelWidth, Top))), White, ESlateDrawEffect::None,
			i == SelectedTrack ? FLinearColor(0.16f, 0.16f, 0.2f) : FLinearColor(0.09f, 0.09f, 0.1f));

		// IK: shade the stretches where the IK weight is above zero.
		if (bIK)
		{
			for (int32 k = 0; k + 1 < Track.Keys.Num(); ++k)
			{
				const float W = FMath::Max(Track.Keys[k].Weight, Track.Keys[k + 1].Weight);
				if (W <= 0.001f) { continue; }
				const float X0 = TimeToX(Track.Keys[k].Time), X1 = TimeToX(Track.Keys[k + 1].Time);
				FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(FMath::Max(1.f, X1 - X0), 6.f), FSlateLayoutTransform(FVector2f(X0, Top + TrackLaneHeight - 8.f))), White, ESlateDrawEffect::None, Base * 0.55f);
			}
		}

		if (i == ExpandedTrack && IsExpanderOpen())
		{
			// The area under the lane belongs to the numeric value rows (in the track column); stripe the time side to match.
			const float PanelTop = Top + TrackLaneHeight + 2.f;
			const float RowH = (ExpanderHeight - 2.f) / 4.f;
			for (int32 r = 0; r < 4; ++r)
			{
				FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size.X - LabelWidth, RowH), FSlateLayoutTransform(FVector2f(LabelWidth, PanelTop + r * RowH))), White, ESlateDrawEffect::None,
					(r & 1) ? FLinearColor(0.075f, 0.075f, 0.085f) : FLinearColor(0.06f, 0.06f, 0.07f));
			}
		}

		// Keys: solid red circles, solid blue when selected (Sequencer style).
		for (int32 k = 0; k < Track.Keys.Num(); ++k)
		{
			const float X = TimeToX(Track.Keys[k].Time);
			const float Y = Top + TrackLaneHeight * 0.5f;
			const bool bSelected = IsKeySelected(i, k);
			const float D = bSelected ? 13.f : 11.f;
			const FLinearColor Color = bSelected ? FLinearColor(0.2f, 0.5f, 1.f) : FLinearColor(0.9f, 0.15f, 0.12f);
			FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(D, D), FSlateLayoutTransform(FVector2f(X - D * 0.5f, Y - D * 0.5f))), Circle ? Circle : White, ESlateDrawEffect::None, Color);
		}
	}
	Out.PopClip();
}

/** The Sequencer-like track column: titles with an expand arrow, key navigation buttons and the area behind the value rows. */
void SAnimStitchTimeline::PaintLeftColumn(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G) const
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	const FVector2f Size = G.GetLocalSize();
	const FSlateBrush* White = FAppStyle::GetBrush("WhiteBrush");
	static const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	static const FSlateFontInfo BoldFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(LabelWidth, Size.Y), FSlateLayoutTransform(FVector2f::ZeroVector)), White, ESlateDrawEffect::None, FLinearColor(0.07f, 0.07f, 0.08f));
	FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(LabelWidth, RulerHeight), FSlateLayoutTransform(FVector2f::ZeroVector)), White, ESlateDrawEffect::None, FLinearColor(0.1f, 0.1f, 0.115f));
	FSlateDrawElement::MakeBox(Out, Layer + 2, G.ToPaintGeometry(FVector2f(1.f, Size.Y), FSlateLayoutTransform(FVector2f(LabelWidth - 1.f, 0.f))), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.8f));
	FSlateDrawElement::MakeText(Out, Layer + 3, G.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(10.f, GetTrackTop() + 24.f))), FString(TEXT("Clips")), BoldFont, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.6f));
	if (IsFootLaneVisible())
	{
		for (int32 Foot = 0; Foot < 2; ++Foot)
		{
			FSlateDrawElement::MakeText(Out, Layer + 3, G.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(10.f, GetLaneTop(Foot) - 2.f))), Foot == 0 ? FString(TEXT("Left foot lock")) : FString(TEXT("Right foot lock")), Font, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.55f));
		}
	}

	const float AreaTop = GetTracksTop();
	Out.PushClip(FSlateClippingZone(G.MakeChild(FVector2f(Size.X, FMath::Max(1.f, Size.Y - AreaTop)), FSlateLayoutTransform(FVector2f(0.f, AreaTop)))));
	for (int32 i = 0; i < TL.Tracks.Num(); ++i)
	{
		const FAnimStitchTrack& Track = TL.Tracks[i];
		const float Top = GetTrackLaneTop(i);
		const bool bIK = Track.Kind == EAnimStitchTrackKind::IKLimb;
		const bool bOpen = i == ExpandedTrack && IsExpanderOpen();
		const FLinearColor Base = bIK ? FLinearColor(0.95f, 0.5f, 0.15f) : (Track.bRotationOnly ? FLinearColor(0.9f, 0.6f, 0.2f) : FLinearColor(0.35f, 0.85f, 0.5f));

		FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(LabelWidth - 1.f, TrackLaneHeight), FSlateLayoutTransform(FVector2f(0.f, Top))), White, ESlateDrawEffect::None,
			i == SelectedTrack ? FLinearColor(0.17f, 0.2f, 0.27f) : FLinearColor(0.11f, 0.11f, 0.125f));
		FSlateDrawElement::MakeBox(Out, Layer + 4, G.ToPaintGeometry(FVector2f(3.f, TrackLaneHeight), FSlateLayoutTransform(FVector2f(LabelWidth - 4.f, Top))), White, ESlateDrawEffect::None, Base);
		FSlateDrawElement::MakeText(Out, Layer + 5, G.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(7.f, Top + 7.f))), FString(bOpen ? TEXT("v") : TEXT(">")), Font, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.8f));
		FString Label = Track.Bone.ToString();
		if (bIK) { Label += TEXT("  (IK)"); }
		else if (Track.bRotationOnly) { Label += TEXT("  (FK)"); }
		FSlateDrawElement::MakeText(Out, Layer + 5, G.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(24.f, Top + 7.f))), Label, BoldFont, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.9f));

		// Previous key, add key at the playhead, next key.
		const TCHAR* Glyphs[3] = { TEXT("<"), TEXT("+"), TEXT(">") };
		for (int32 b = 0; b < 3; ++b)
		{
			const FVector2f Pos(LabelWidth - 4.f - 3 * 24.f + b * 24.f, Top + 4.f);
			FSlateDrawElement::MakeBox(Out, Layer + 5, G.ToPaintGeometry(FVector2f(22.f, TrackLaneHeight - 8.f), FSlateLayoutTransform(Pos)), White, ESlateDrawEffect::None, FLinearColor(0.2f, 0.2f, 0.25f));
			FSlateDrawElement::MakeText(Out, Layer + 6, G.ToPaintGeometry(Size, FSlateLayoutTransform(Pos + FVector2f(8.f, 2.f))), FString(Glyphs[b]), Font, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.85f));
		}

		if (bOpen)
		{
			FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(LabelWidth - 1.f, ExpanderHeight - 2.f), FSlateLayoutTransform(FVector2f(0.f, Top + TrackLaneHeight + 2.f))), White, ESlateDrawEffect::None, FLinearColor(0.085f, 0.085f, 0.1f));
		}
	}
	Out.PopClip();
}
// ---------------------------------------------------------------------------------------------

bool SAnimStitchTimeline::WarnIfTrackExists(FName Bone, EAnimStitchTrackKind Kind)
{
	for (int32 i = 0; i < Asset->Timeline.Tracks.Num(); ++i)
	{
		if (Asset->Timeline.Tracks[i].Bone == Bone && Asset->Timeline.Tracks[i].Kind == Kind)
		{
			FNotificationInfo Info(FText::Format(LOCTEXT("DuplicateTrack", "'{0}' is already on the track list."), FText::FromName(Bone)));
			Info.ExpireDuration = 4.f;
			Info.bUseLargeFont = false;
			FSlateNotificationManager::Get().AddNotification(Info);
			SelectedTrack = i;
			SelectedKey = INDEX_NONE;
			return true;
		}
	}
	return false;
}

void SAnimStitchTimeline::AddBoneTrack(FName Bone, bool bRotationOnly, int32 GroupId)
{
	if (WarnIfTrackExists(Bone, EAnimStitchTrackKind::Bone)) { return; }
	FAnimStitchTrack Track;
	Track.Kind = EAnimStitchTrackKind::Bone;
	Track.Bone = Bone;
	Track.bRotationOnly = bRotationOnly;
	Track.GroupId = GroupId;
	const FScopedTransaction Tx(LOCTEXT("AddBoneTrackTx", "Add Bone Track"));
	SelectedTrack = Asset->AddTrack(Track);
	SelectedKey = INDEX_NONE;
}

void SAnimStitchTimeline::AddLimb(bool bLeft, bool bLeg, bool bIK)
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton) { return; }
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	const FName Effector = bLeg ? FAnimStitchFootLock::GuessFootBone(Ref, bLeft) : FAnimStitchManualEdits::GuessHandBone(Ref, bLeft);
	const FAnimStitchFootChain Chain = FAnimStitchManualEdits::ChainFor(Ref, Effector);
	if (!Chain.IsValid())
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("NoLimb", "Couldn't find the {0} {1} on this skeleton. Use Add Bone Track > Core Bone > Other Bone... instead."),
			bLeft ? LOCTEXT("LeftWord", "left") : LOCTEXT("RightWord", "right"), bLeg ? LOCTEXT("LegWord", "leg") : LOCTEXT("ArmWord", "arm")));
		Info.ExpireDuration = 6.f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	if (bIK ? WarnIfTrackExists(Effector, EAnimStitchTrackKind::IKLimb) : (WarnIfTrackExists(Ref.GetBoneName(Chain.Thigh), EAnimStitchTrackKind::Bone) || WarnIfTrackExists(Ref.GetBoneName(Chain.Calf), EAnimStitchTrackKind::Bone) || WarnIfTrackExists(Ref.GetBoneName(Chain.Foot), EAnimStitchTrackKind::Bone)))
	{
		return;
	}
	const FScopedTransaction Tx(LOCTEXT("AddLimbTx", "Add Limb Tracks"));
	if (bIK)
	{
		FAnimStitchTrack Track;
		Track.Kind = EAnimStitchTrackKind::IKLimb;
		Track.Bone = Effector;
		SelectedTrack = Asset->AddTrack(Track);
	}
	else
	{
		int32 Group = 1;
		for (const FAnimStitchTrack& T : Asset->Timeline.Tracks) { Group = FMath::Max(Group, T.GroupId + 1); }
		for (const int32 Joint : { Chain.Thigh, Chain.Calf, Chain.Foot })
		{
			FAnimStitchTrack Track;
			Track.Kind = EAnimStitchTrackKind::Bone;
			Track.Bone = Ref.GetBoneName(Joint);
			Track.bRotationOnly = true;
			Track.bKeepFeetPlanted = false; // an FK thigh/knee rotation is meant to move the leg
			Track.GroupId = Group;
			SelectedTrack = Asset->AddTrack(Track);
		}
	}
	SelectedKey = INDEX_NONE;
}

void SAnimStitchTimeline::PickOtherBone()
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton) { return; }
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	TSharedRef<TArray<TSharedPtr<FString>>> Items = MakeShared<TArray<TSharedPtr<FString>>>();
	for (int32 b = 0; b < Ref.GetNum(); ++b) { Items->Add(MakeShared<FString>(Ref.GetBoneName(b).ToString())); }
	TSharedRef<TSharedPtr<FString>> Chosen = MakeShared<TSharedPtr<FString>>();

	TSharedRef<SWindow> Window = SNew(SWindow).Title(LOCTEXT("PickBoneTitle", "Add Bone Track")).ClientSize(FVector2D(380.0, 130.0)).SupportsMinimize(false).SupportsMaximize(false);
	Window->SetContent(
		SNew(SBox).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[SNew(STextBlock).Text(LOCTEXT("PickBoneHint", "Choose the bone to correct (additive offsets):"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SSearchableComboBox)
				.OptionsSource(&Items.Get())
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(*Item)); })
				.OnSelectionChanged_Lambda([Chosen](TSharedPtr<FString> Item, ESelectInfo::Type) { *Chosen = Item; })
				[
					SNew(STextBlock).Text_Lambda([Chosen]() { return Chosen->IsValid() ? FText::FromString(**Chosen) : LOCTEXT("PickBoneOther", "Pick a bone..."); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SButton).Text(LOCTEXT("AddTrackOk", "Add Track"))
				.OnClicked_Lambda([this, Chosen, WeakWindow = TWeakPtr<SWindow>(Window)]()
				{
					if (Chosen->IsValid()) { AddBoneTrack(FName(**Chosen), false, 0); }
					if (TSharedPtr<SWindow> W = WeakWindow.Pin()) { W->RequestDestroyWindow(); }
					return FReply::Handled();
				})
			]
		]);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SAnimStitchTimeline::OpenAddTrackMenu(const FPointerEvent& Event)
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	FMenuBuilder Menu(true, nullptr);
	if (!Skeleton)
	{
		Menu.AddMenuEntry(LOCTEXT("NoSkeletonYet", "Add clips to the timeline first"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
	}
	else
	{
		const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
		const TArray<FName> Core = FAnimStitchManualEdits::GetCoreBoneSuggestions(Ref);
		Menu.BeginSection("BoneTracks", LOCTEXT("AddBoneTrackSection", "Add Bone Track"));
		Menu.AddSubMenu(LOCTEXT("CoreBoneMenu", "Core Bone"), LOCTEXT("CoreBoneMenuTip", "Pelvis, spine, head...: one control per bone, additive offsets"),
			FNewMenuDelegate::CreateLambda([this, Core](FMenuBuilder& Sub)
			{
				for (const FName& Name : Core)
				{
					Sub.AddMenuEntry(FText::FromName(Name), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Name]() { AddBoneTrack(Name, false, 0); })));
				}
				Sub.AddSeparator();
				Sub.AddMenuEntry(LOCTEXT("OtherBone", "Other Bone..."), LOCTEXT("OtherBoneTip", "Pick any bone of the skeleton"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { PickOtherBone(); })));
			}));
		Menu.AddSubMenu(LOCTEXT("LimbMenu", "Limb"), LOCTEXT("LimbMenuTip", "Arms and legs, FK (rotate each joint) or IK (effector + pole vector)"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& Sub)
			{
				auto Limb = [this, &Sub](const FText& Name, bool bLeft, bool bLeg)
				{
					Sub.AddSubMenu(Name, FText::GetEmpty(), FNewMenuDelegate::CreateLambda([this, bLeft, bLeg](FMenuBuilder& Choice)
					{
						Choice.AddMenuEntry(LOCTEXT("LimbFK", "FK: a control on each joint (rotate)"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, bLeft, bLeg]() { AddLimb(bLeft, bLeg, false); })));
						Choice.AddMenuEntry(LOCTEXT("LimbIK", "IK: end effector + pole vector (free target)"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, bLeft, bLeg]() { AddLimb(bLeft, bLeg, true); })));
					}));
				};
				Limb(LOCTEXT("LeftLeg", "Left Leg"), true, true);
				Limb(LOCTEXT("RightLeg", "Right Leg"), false, true);
				Limb(LOCTEXT("LeftArm", "Left Arm"), true, false);
				Limb(LOCTEXT("RightArm", "Right Arm"), false, false);
			}));
		Menu.EndSection();
	}
	const FWidgetPath Path = Event.GetEventPath() != nullptr ? *Event.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), Path, Menu.MakeWidget(), Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

void SAnimStitchTimeline::OpenTrackMenu(const FPointerEvent& Event, int32 TrackIndex, int32 KeyIndex, double Time)
{
	FMenuBuilder Menu(true, nullptr);
	const FAnimStitchTrack& Track = Asset->Timeline.Tracks[TrackIndex];
	const bool bIK = Track.Kind == EAnimStitchTrackKind::IKLimb;

	if (Track.Keys.Num() > 0)
	{
		Menu.AddMenuEntry(LOCTEXT("KeyAtPlayhead", "Add Key at Playhead"), LOCTEXT("KeyAtPlayheadTip", "Key the current interpolated value"), FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, TrackIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("AddKeyTx", "Add Key"));
				FAnimStitchTrackKey Key = FAnimStitchManualEdits::Sample(Asset->Timeline.Tracks[TrackIndex], Playhead);
				Key.Time = Playhead;
				Asset->SetTrackKey(TrackIndex, Key);
			})));
	}
	if (bIK && Track.Keys.Num() > 0)
	{
		Menu.AddMenuEntry(LOCTEXT("ReleaseIK", "Release IK Here"), LOCTEXT("ReleaseIKTip", "Key weight 0 at the playhead: the IK eases off over 0.1 s and the limb returns to the animation"), FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, TrackIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("ReleaseIKTx", "Release IK"));
				FAnimStitchTrackKey Key = FAnimStitchManualEdits::Sample(Asset->Timeline.Tracks[TrackIndex], Playhead);
				Key.Time = Playhead;
				Key.Weight = 0.f;
				Asset->SetTrackKey(TrackIndex, Key);
			})));
	}
	if (KeyIndex != INDEX_NONE)
	{
		Menu.AddSeparator();
		if (bIK)
		{
			Menu.AddMenuEntry(LOCTEXT("KeyWeight1", "Key Weight 1 (IK on, plants on the target)"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, KeyIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("KeyWeightTx", "Set Key Weight"));
				Asset->SetTrackKeyWeight(TrackIndex, KeyIndex, 1.f);
			})));
			Menu.AddMenuEntry(LOCTEXT("KeyWeight0", "Key Weight 0 (release)"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, KeyIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("KeyWeightTx", "Set Key Weight"));
				Asset->SetTrackKeyWeight(TrackIndex, KeyIndex, 0.f);
			})));
		}
		Menu.AddMenuEntry(LOCTEXT("DeleteKey", "Delete Key"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, KeyIndex]()
		{
			const FScopedTransaction Tx(LOCTEXT("DeleteKeyTx", "Delete Key"));
			Asset->RemoveTrackKey(TrackIndex, KeyIndex);
			SelectedKey = INDEX_NONE;
		})));
	}
	Menu.AddSeparator();
	Menu.AddMenuEntry(LOCTEXT("RemoveTrack", "Remove Track"), LOCTEXT("RemoveTrackTip", "Remove this lane and all its keys"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex]()
	{
		const FScopedTransaction Tx(LOCTEXT("RemoveTrackTx", "Remove Track"));
		Asset->RemoveTrack(TrackIndex);
		ExpandedTrack = INDEX_NONE;
		SelectedTrack = INDEX_NONE;
		SelectedKey = INDEX_NONE;
	})));
	const FWidgetPath Path = Event.GetEventPath() != nullptr ? *Event.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), Path, Menu.MakeWidget(), Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

#undef LOCTEXT_NAMESPACE



