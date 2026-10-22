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
#include "Widgets/Input/SCheckBox.h"
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
		const FLinearColor Base = Track.Kind == EAnimStitchTrackKind::IKPole ? FLinearColor(0.7f, 0.4f, 1.f) : bIK ? FLinearColor(0.95f, 0.5f, 0.15f) : (Track.bRotationOnly ? FLinearColor(0.9f, 0.6f, 0.2f) : FLinearColor(0.35f, 0.85f, 0.5f));

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

		if (IsTrackExpanded(i))
		{
			// The area under the lane belongs to the numeric value rows (in the track column); stripe the time side to match.
			const float PanelTop = Top + TrackLaneHeight + 2.f;
			const float RowH = (ExpanderHeight - 2.f) / 4.f;
			for (int32 r = 0; r < 4; ++r)
			{
				FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size.X - LabelWidth, RowH), FSlateLayoutTransform(FVector2f(LabelWidth, PanelTop + r * RowH))), White, ESlateDrawEffect::None,
					(r & 1) ? FLinearColor(0.075f, 0.075f, 0.085f) : FLinearColor(0.06f, 0.06f, 0.07f));
			}
			if (Track.Kind == EAnimStitchTrackKind::Bone)
			{
				// One sub-lane per channel (Translation, Rotation, Scale): each holds only its own keys. A locked lane is dimmed (keys visible, not editable).
				const uint8 Applicable = FAnimStitchManualEdits::GetApplicableChannels(Track);
				const uint8 Editable = FAnimStitchManualEdits::GetEditableChannels(Track);
				for (int32 Row = 0; Row < 3; ++Row)
				{
					const uint8 Bit = ChannelBitForRow(Row);
					const float RowTop = PanelTop + Row * RowH;
					const bool bLocked = (Editable & Bit) == 0;
					if (!(Applicable & Bit))
					{
						FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(Size.X - LabelWidth, RowH), FSlateLayoutTransform(FVector2f(LabelWidth, RowTop))), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.55f));
						continue;
					}
					if (bLocked)
					{
						FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(Size.X - LabelWidth, RowH), FSlateLayoutTransform(FVector2f(LabelWidth, RowTop))), White, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.28f));
					}
					for (int32 k = 0; k < Track.Keys.Num(); ++k)
					{
						if (!(Track.Keys[k].Channels & Bit)) { continue; }
						const float KX = TimeToX(Track.Keys[k].Time);
						const float KY = RowTop + RowH * 0.5f;
						const bool bKeySelected = IsKeySelected(i, k, Bit);
						const float KD = bKeySelected ? 11.f : 9.f;
						FLinearColor KColor = bKeySelected ? FLinearColor(0.2f, 0.5f, 1.f) : FLinearColor(0.9f, 0.15f, 0.12f);
						if (bLocked) { KColor = KColor * 0.55f; KColor.A = 1.f; }
						FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(KD, KD), FSlateLayoutTransform(FVector2f(KX - KD * 0.5f, KY - KD * 0.5f))), Circle ? Circle : White, ESlateDrawEffect::None, KColor);
					}
				}
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
		const bool bOpen = IsTrackExpanded(i);
		const FLinearColor Base = Track.Kind == EAnimStitchTrackKind::IKPole ? FLinearColor(0.7f, 0.4f, 1.f) : bIK ? FLinearColor(0.95f, 0.5f, 0.15f) : (Track.bRotationOnly ? FLinearColor(0.9f, 0.6f, 0.2f) : FLinearColor(0.35f, 0.85f, 0.5f));

		FSlateDrawElement::MakeBox(Out, Layer + 3, G.ToPaintGeometry(FVector2f(LabelWidth - 1.f, TrackLaneHeight), FSlateLayoutTransform(FVector2f(0.f, Top))), White, ESlateDrawEffect::None,
			i == SelectedTrack ? FLinearColor(0.17f, 0.2f, 0.27f) : FLinearColor(0.11f, 0.11f, 0.125f));
		FSlateDrawElement::MakeBox(Out, Layer + 4, G.ToPaintGeometry(FVector2f(3.f, TrackLaneHeight), FSlateLayoutTransform(FVector2f(LabelWidth - 4.f, Top))), White, ESlateDrawEffect::None, Base);
		const float Indent = Track.Kind == EAnimStitchTrackKind::IKPole ? 18.f : 0.f; // the pole lane sits indented under its IK track
		if (Indent > 0.f) { FSlateDrawElement::MakeBox(Out, Layer + 4, G.ToPaintGeometry(FVector2f(1.f, TrackLaneHeight), FSlateLayoutTransform(FVector2f(12.f, Top))), White, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.25f)); }
		FSlateDrawElement::MakeText(Out, Layer + 5, G.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(7.f + Indent, Top + 7.f))), FString(bOpen ? TEXT("v") : TEXT(">")), Font, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.8f));
		FString Label = Track.Kind == EAnimStitchTrackKind::RootOffset ? FString(TEXT("Root - motion offset")) : Track.Bone.ToString();
		if (Track.Kind == EAnimStitchTrackKind::IKPole) { Label += TEXT("  pole"); }
		if (bIK) { Label += TEXT("  (IK)"); }
		else if (Track.bRotationOnly) { Label += TEXT("  (FK)"); }
		if (Track.Kind == EAnimStitchTrackKind::Bone && Track.Mode == EAnimStitchTrackMode::Absolute) { Label += TEXT("  [ABS]"); } // absolute tracks are tagged so they stand out from additive ones
		FSlateDrawElement::MakeText(Out, Layer + 5, G.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(24.f + Indent, Top + 7.f))), Label, BoldFont, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.9f));

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

bool SAnimStitchTimeline::HasTrackFor(FName Bone, EAnimStitchTrackKind Kind) const
{
	for (const FAnimStitchTrack& Track : Asset->Timeline.Tracks)
	{
		if (Track.Bone == Bone && Track.Kind == Kind) { return true; }
	}
	return false;
}

FName SAnimStitchTimeline::GetRootBoneName() const
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	return Skeleton && Skeleton->GetReferenceSkeleton().GetNum() > 0 ? Skeleton->GetReferenceSkeleton().GetBoneName(0) : FName();
}

bool SAnimStitchTimeline::HasRootOffsetTrack() const
{
	return Asset->Timeline.Tracks.ContainsByPredicate([](const FAnimStitchTrack& T) { return T.Kind == EAnimStitchTrackKind::RootOffset; });
}

void SAnimStitchTimeline::AddRootOffsetTrack()
{
	const FName Root = GetRootBoneName();
	if (Root.IsNone() || HasRootOffsetTrack()) { return; } // never a second root track
	FAnimStitchTrack Track;
	Track.Kind = EAnimStitchTrackKind::RootOffset;
	Track.Bone = Root;
	const FScopedTransaction Tx(LOCTEXT("AddRootTrackTx", "Add Root Offset Track"));
	SelectedTrack = Asset->AddTrack(Track);
	++TrackSelectionSerial;
	SelectedKey = INDEX_NONE;
	SelectedKeys.Reset();
}

FAnimStitchTrackKey SAnimStitchTimeline::MakeKeyAtPlayhead(int32 TrackIndex) const
{
	const FAnimStitchTrack& Track = Asset->Timeline.Tracks[TrackIndex];
	FAnimStitchTrackKey Key = Track.Keys.Num() > 0 ? FAnimStitchManualEdits::Sample(Track, Playhead) : FAnimStitchTrackKey();
	if (LiveKeyProvider)
	{
		FAnimStitchTrackKey Live;
		if (Track.Kind == EAnimStitchTrackKind::Bone && Track.Mode == EAnimStitchTrackMode::Absolute)
		{
			// Absolute: key what the bone currently evaluates to (full transform), so keying never pops.
			if (LiveKeyProvider(TrackIndex, Live)) { Key.Translation = Live.Translation; Key.Rotation = Live.Rotation; Key.Scale = Live.Scale; }
		}
		else if (Track.Kind == EAnimStitchTrackKind::IKLimb || Track.Kind == EAnimStitchTrackKind::IKPole)
		{
			if (LiveKeyProvider(TrackIndex, Live))
			{
				if (Track.Kind == EAnimStitchTrackKind::IKPole) { Key.Translation = Live.Pole; }
				else if (Key.Weight < 1e-3f) { Key.Translation = Live.Translation; Key.Rotation = Live.Rotation; Key.Pole = Live.Pole; }
			}
		}
	}
	Key.Time = Playhead;
	return Key;
}

void SAnimStitchTimeline::AddBoneTrack(FName Bone, bool bRotationOnly, int32 GroupId, EAnimStitchTrackMode Mode)
{
	if (!Bone.IsNone() && Bone == GetRootBoneName()) { AddRootOffsetTrack(); return; } // the root has its own behaviour; no FK / IK / mode choice
	if (HasTrackFor(Bone, EAnimStitchTrackKind::Bone)) { return; } // the pickers do not offer these; nothing to add
	FAnimStitchTrack Track;
	Track.Kind = EAnimStitchTrackKind::Bone;
	Track.Bone = Bone;
	Track.bRotationOnly = bRotationOnly;
	Track.GroupId = GroupId;
	Track.Mode = Mode;
	const FScopedTransaction Tx(LOCTEXT("AddBoneTrackTx", "Add Bone Track"));
	SelectedTrack = Asset->AddTrack(Track);
	++TrackSelectionSerial;
	SelectedKey = INDEX_NONE;
	SelectedKeys.Reset();
}

bool SAnimStitchTimeline::LimbTrackExists(bool bLeft, bool bLeg, bool bIK) const
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton) { return false; }
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	const FName Effector = bLeg ? FAnimStitchFootLock::GuessFootBone(Ref, bLeft) : FAnimStitchManualEdits::GuessHandBone(Ref, bLeft);
	const FAnimStitchFootChain Chain = FAnimStitchManualEdits::ChainFor(Ref, Effector);
	if (!Chain.IsValid()) { return false; }
	if (bIK) { return HasTrackFor(Effector, EAnimStitchTrackKind::IKLimb); }
	return HasTrackFor(Ref.GetBoneName(Chain.Thigh), EAnimStitchTrackKind::Bone) || HasTrackFor(Ref.GetBoneName(Chain.Calf), EAnimStitchTrackKind::Bone) || HasTrackFor(Ref.GetBoneName(Chain.Foot), EAnimStitchTrackKind::Bone);
}

void SAnimStitchTimeline::AddLimbForEffector(FName Effector, bool bIK, EAnimStitchTrackMode Mode)
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton) { return; }
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	const FAnimStitchFootChain Chain = FAnimStitchManualEdits::ChainFor(Ref, Effector);
	if (!Chain.IsValid())
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("NoLimb", "Couldn't build a limb ending at '{0}' on this skeleton. Use a single bone track instead."), FText::FromName(Effector)));
		Info.ExpireDuration = 6.f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	const FScopedTransaction Tx(LOCTEXT("AddLimbTx", "Add Limb Tracks"));
	if (bIK)
	{
		if (HasTrackFor(Effector, EAnimStitchTrackKind::IKLimb)) { return; }
		FAnimStitchTrack Track;
		Track.Kind = EAnimStitchTrackKind::IKLimb;
		Track.Bone = Effector;
		SelectedTrack = Asset->AddTrack(Track);
		++TrackSelectionSerial;
		// The pole vector gets its own lane right under the IK track; its control defaults to a sphere.
		if (!HasTrackFor(Effector, EAnimStitchTrackKind::IKPole))
		{
			FAnimStitchTrack PoleTrack;
			PoleTrack.Kind = EAnimStitchTrackKind::IKPole;
			PoleTrack.Bone = Effector;
			PoleTrack.ControlStyle.Shape = EAnimStitchControlShape::Sphere;
			Asset->AddTrack(PoleTrack);
		}
	}
	else
	{
		int32 Group = 1;
		for (const FAnimStitchTrack& T : Asset->Timeline.Tracks) { Group = FMath::Max(Group, T.GroupId + 1); }
		for (const int32 Joint : { Chain.Thigh, Chain.Calf, Chain.Foot })
		{
			if (HasTrackFor(Ref.GetBoneName(Joint), EAnimStitchTrackKind::Bone)) { continue; }
			FAnimStitchTrack Track;
			Track.Kind = EAnimStitchTrackKind::Bone;
			Track.Bone = Ref.GetBoneName(Joint);
			Track.bRotationOnly = true;
			Track.bKeepFeetPlanted = false;
			Track.GroupId = Group;
			Track.Mode = Mode;
			SelectedTrack = Asset->AddTrack(Track);
	++TrackSelectionSerial;
		}
	}
	SelectedKey = INDEX_NONE;
	SelectedKeys.Reset();
}

void SAnimStitchTimeline::AddLimb(bool bLeft, bool bLeg, bool bIK)
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton) { return; }
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	const FName Effector = bLeg ? FAnimStitchFootLock::GuessFootBone(Ref, bLeft) : FAnimStitchManualEdits::GuessHandBone(Ref, bLeft);
	if (!FAnimStitchManualEdits::ChainFor(Ref, Effector).IsValid())
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("NoLimbSide", "Couldn't find the {0} {1} on this skeleton. Use Add Core Bone > Other Bone... instead."),
			bLeft ? LOCTEXT("LeftWord", "left") : LOCTEXT("RightWord", "right"), bLeg ? LOCTEXT("LegWord", "leg") : LOCTEXT("ArmWord", "arm")));
		Info.ExpireDuration = 6.f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	AddLimbForEffector(Effector, bIK);
}

void SAnimStitchTimeline::PickOtherBone()
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton) { return; }
	const FReferenceSkeleton& Ref = Skeleton->GetReferenceSkeleton();
	// Bones already on the track list are not offered, so duplicates cannot be picked.
	TSharedRef<TArray<TSharedPtr<FString>>> Items = MakeShared<TArray<TSharedPtr<FString>>>();
	for (int32 b = 0; b < Ref.GetNum(); ++b)
	{
		if (b == 0 && HasRootOffsetTrack()) { continue; } // the root track exists: the root is no longer offered
		if (!HasTrackFor(Ref.GetBoneName(b), EAnimStitchTrackKind::Bone)) { Items->Add(MakeShared<FString>(Ref.GetBoneName(b).ToString())); }
	}
	TSharedRef<TSharedPtr<FString>> Chosen = MakeShared<TSharedPtr<FString>>();
	TSharedRef<int32> Mode = MakeShared<int32>(0); // 0 = single bone, 1 = FK limb, 2 = IK limb
	TSharedRef<int32> TrackMode = MakeShared<int32>(0); // 0 = additive offsets, 1 = absolute transforms
	const FName RootBone = GetRootBoneName();
	auto IsRoot = [RootBone](const FString& Name) { return !RootBone.IsNone() && FName(*Name) == RootBone; };

	// Hands and feet can become a whole limb (FK joints or IK), the same choice as the Limb menu.
	auto IsEffector = [Skeleton](const FString& Name)
	{
		const FString Lower = Name.ToLower();
		if (Lower.Contains(TEXT("twist")) || Lower.Contains(TEXT("ik")) || Lower.Contains(TEXT("corrective")) || Lower.Contains(TEXT("helper"))) { return false; }
		return (Lower.Contains(TEXT("foot")) || Lower.Contains(TEXT("hand"))) && FAnimStitchManualEdits::ChainFor(Skeleton->GetReferenceSkeleton(), FName(*Name)).IsValid();
	};
	auto TrackModeButton = [TrackMode](const FText& Label, const FText& Tip, int32 Value)
	{
		return SNew(SCheckBox).Style(FAppStyle::Get(), "RadioButton")
			.ToolTipText(Tip)
			.IsChecked_Lambda([TrackMode, Value]() { return *TrackMode == Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([TrackMode, Value](ECheckBoxState State) { if (State == ECheckBoxState::Checked) { *TrackMode = Value; } })
			[
				SNew(STextBlock).Text(Label)
			];
	};
	auto ModeButton = [Mode](const FText& Label, int32 Value)
	{
		return SNew(SCheckBox).Style(FAppStyle::Get(), "RadioButton")
			.ToolTipText(Value == 0 ? LOCTEXT("ModeSingleTip", "Add just this bone as one track of additive offsets") : Value == 1 ? LOCTEXT("ModeFKTip", "Add a rotate-only track for each joint of the limb (thigh, calf, foot or upper arm, forearm, hand)") : LOCTEXT("ModeIKTip", "Add an IK track for the end effector, with its own pole-vector lane"))
			.IsChecked_Lambda([Mode, Value]() { return *Mode == Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([Mode, Value](ECheckBoxState State) { if (State == ECheckBoxState::Checked) { *Mode = Value; } })
			[
				SNew(STextBlock).Text(Label)
			];
	};

	TSharedRef<SWindow> Window = SNew(SWindow).Title(LOCTEXT("PickBoneTitle", "Add Bone Track")).ClientSize(FVector2D(420.0, 300.0)).SupportsMinimize(false).SupportsMaximize(false);
	Window->SetContent(
		SNew(SBox).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[SNew(STextBlock).Text(LOCTEXT("PickBoneHint", "Choose the bone to correct:"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SSearchableComboBox)
				.ToolTipText(LOCTEXT("PickBoneComboTip", "The bone to correct. Bones that already have a track are not listed. Type to search."))
				.OptionsSource(&Items.Get())
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(*Item)); })
				.OnSelectionChanged_Lambda([Chosen, Mode](TSharedPtr<FString> Item, ESelectInfo::Type) { *Chosen = Item; *Mode = 0; })
				[
					SNew(STextBlock).Text_Lambda([Chosen]() { return Chosen->IsValid() ? FText::FromString(**Chosen) : LOCTEXT("PickBoneOther", "Pick a bone..."); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SVerticalBox)
				.Visibility_Lambda([Chosen, IsRoot]() { return Chosen->IsValid() && IsRoot(**Chosen) ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text(LOCTEXT("RootPickedHint", "The root is added as the Root - motion offset track: additive translation + yaw, applied with the blend before foot lock. There is no FK / IK or Additive / Absolute choice for it."))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SVerticalBox)
				.Visibility_Lambda([Chosen, Mode, IsRoot]() { return Chosen->IsValid() && !IsRoot(**Chosen) && *Mode != 2 ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)[SNew(STextBlock).Text(LOCTEXT("TrackModeHint", "Track mode (fixed for the life of the track):"))]
				+ SVerticalBox::Slot().AutoHeight()[TrackModeButton(LOCTEXT("ModeAdditive", "Additive: keys are offsets on the animation"), LOCTEXT("ModeAdditiveTip", "The animation keeps playing; keys add offsets to it (default)"), 0)]
				+ SVerticalBox::Slot().AutoHeight()[TrackModeButton(LOCTEXT("ModeAbsolute", "Absolute: keys are full transforms"), LOCTEXT("ModeAbsoluteTip", "Keys hold the bone's full transform and replace the animation between the first and last key"), 1)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SVerticalBox)
				.Visibility_Lambda([Chosen, IsEffector]() { return Chosen->IsValid() && IsEffector(**Chosen) ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)[SNew(STextBlock).Text(LOCTEXT("PickLimbHint", "This is a hand / foot. Add it as:"))]
				+ SVerticalBox::Slot().AutoHeight()[ModeButton(LOCTEXT("ModeSingle", "Single bone (additive offsets)"), 0)]
				+ SVerticalBox::Slot().AutoHeight()[ModeButton(LOCTEXT("ModeFK", "FK limb: a control on each joint (rotate)"), 1)]
				+ SVerticalBox::Slot().AutoHeight()[ModeButton(LOCTEXT("ModeIK", "IK limb: end effector + pole vector"), 2)]
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[SNullWidget::NullWidget]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SButton).Text(LOCTEXT("AddTrackOk", "Add Track")).ToolTipText(LOCTEXT("AddTrackOkTip", "Create the track for the chosen bone"))
				.IsEnabled_Lambda([Chosen]() { return Chosen->IsValid(); })
				.OnClicked_Lambda([this, Chosen, Mode, TrackMode, IsEffector, IsRoot, WeakWindow = TWeakPtr<SWindow>(Window)]()
				{
					if (Chosen->IsValid())
					{
						const FName Bone(**Chosen);
						const EAnimStitchTrackMode TrackModeValue = *TrackMode == 1 ? EAnimStitchTrackMode::Absolute : EAnimStitchTrackMode::Additive;
							if (IsRoot(**Chosen)) { AddRootOffsetTrack(); }
							else if (*Mode == 0 || !IsEffector(**Chosen)) { AddBoneTrack(Bone, false, 0, TrackModeValue); }
						else { AddLimbForEffector(Bone, *Mode == 2, TrackModeValue); }
					}
					if (TSharedPtr<SWindow> W = WeakWindow.Pin()) { W->RequestDestroyWindow(); }
					return FReply::Handled();
				})
			]
		]);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SAnimStitchTimeline::BuildCoreBoneMenu(FMenuBuilder& Menu)
{
	const USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton)
	{
		Menu.AddMenuEntry(LOCTEXT("NoSkeletonYet", "Add clips to the timeline first"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
		return;
	}
	for (const FName& Name : FAnimStitchManualEdits::GetCoreBoneSuggestions(Skeleton->GetReferenceSkeleton()))
	{
		if (HasTrackFor(Name, EAnimStitchTrackKind::Bone)) { continue; } // already on the list
		Menu.AddSubMenu(FText::FromName(Name), FText::GetEmpty(), FNewMenuDelegate::CreateLambda([this, Name](FMenuBuilder& Choice)
		{
			Choice.AddMenuEntry(LOCTEXT("CoreModeAdditive", "Additive: keys are offsets on the animation"), LOCTEXT("CoreModeAdditiveTip", "The animation keeps playing and your keys add offsets on top of it (default)"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Name]() { AddBoneTrack(Name, false, 0, EAnimStitchTrackMode::Additive); })));
			Choice.AddMenuEntry(LOCTEXT("CoreModeAbsolute", "Absolute: keys are full transforms"), LOCTEXT("CoreModeAbsoluteTip", "Your keys hold the bone's full transform relative to the root and replace the animation between the first and last key"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Name]() { AddBoneTrack(Name, false, 0, EAnimStitchTrackMode::Absolute); })));
		}));
	}
	Menu.AddSeparator();
	Menu.AddMenuEntry(LOCTEXT("OtherBone", "Other Bone..."), LOCTEXT("OtherBoneTip", "Pick any bone of the skeleton (hands and feet can be added as FK or IK limbs)"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { PickOtherBone(); })));
}

void SAnimStitchTimeline::BuildLimbMenu(FMenuBuilder& Menu)
{
	if (!Asset->GetSkeleton())
	{
		Menu.AddMenuEntry(LOCTEXT("NoSkeletonYet2", "Add clips to the timeline first"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
		return;
	}
	auto Limb = [this, &Menu](const FText& Name, bool bLeft, bool bLeg)
	{
		Menu.AddSubMenu(Name, FText::GetEmpty(), FNewMenuDelegate::CreateLambda([this, bLeft, bLeg](FMenuBuilder& Choice)
		{
			Choice.AddMenuEntry(LOCTEXT("LimbFK", "FK: a control on each joint (rotate)"), LOCTEXT("LimbFKTip", "A rotate-only track and control on each joint of the limb"), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, bLeft, bLeg]() { AddLimb(bLeft, bLeg, false); }), FCanExecuteAction::CreateLambda([this, bLeft, bLeg]() { return !LimbTrackExists(bLeft, bLeg, false); })));
			Choice.AddMenuEntry(LOCTEXT("LimbIK", "IK: end effector + pole vector (free target)"), LOCTEXT("LimbIKTip", "An IK track for the end effector plus a pole-vector lane, so the hand or foot can be placed freely"), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, bLeft, bLeg]() { AddLimb(bLeft, bLeg, true); }), FCanExecuteAction::CreateLambda([this, bLeft, bLeg]() { return !LimbTrackExists(bLeft, bLeg, true); })));
		}));
	};
	Limb(LOCTEXT("LeftLeg", "Left Leg"), true, true);
	Limb(LOCTEXT("RightLeg", "Right Leg"), false, true);
	Limb(LOCTEXT("LeftArm", "Left Arm"), true, false);
	Limb(LOCTEXT("RightArm", "Right Arm"), false, false);
}

void SAnimStitchTimeline::OpenAddTrackMenu(const FPointerEvent& Event)
{
	FMenuBuilder Menu(true, nullptr);
	Menu.BeginSection("BoneTracks", LOCTEXT("AddBoneTrackSection", "Add Bone Track"));
	Menu.AddSubMenu(LOCTEXT("CoreBoneMenu", "Core Bone"), LOCTEXT("CoreBoneMenuTip", "Pelvis, spine, head...: one control per bone, additive offsets"), FNewMenuDelegate::CreateSP(this, &SAnimStitchTimeline::BuildCoreBoneMenu));
	Menu.AddSubMenu(LOCTEXT("LimbMenu", "Limb"), LOCTEXT("LimbMenuTip", "Arms and legs, FK (rotate each joint) or IK (effector + pole vector)"), FNewMenuDelegate::CreateSP(this, &SAnimStitchTimeline::BuildLimbMenu));
	Menu.AddMenuEntry(LOCTEXT("RootOffsetEntry", "Root - motion offset"), LOCTEXT("RootOffsetEntryTip", "One keyframe lane of additive translation + yaw on the root, applied with the blend before foot lock (planted feet follow the corrected path)"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { AddRootOffsetTrack(); }), FCanExecuteAction::CreateLambda([this]() { return Asset->GetSkeleton() != nullptr && !HasRootOffsetTrack(); })));
	Menu.EndSection();
	const FWidgetPath Path = Event.GetEventPath() != nullptr ? *Event.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), Path, Menu.MakeWidget(), Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}
void SAnimStitchTimeline::OpenTrackMenu(const FPointerEvent& Event, int32 TrackIndex, int32 KeyIndex, double Time, uint8 Channel)
{
	FMenuBuilder Menu(true, nullptr);
	const FAnimStitchTrack& Track = Asset->Timeline.Tracks[TrackIndex];
	const bool bIK = Track.Kind == EAnimStitchTrackKind::IKLimb;

	Menu.BeginSection("TrackExpansion", LOCTEXT("TrackExpansionSection", "Tracks"));
	Menu.AddMenuEntry(LOCTEXT("ExpandAllTracks", "Expand All"), LOCTEXT("ExpandAllTip", "Open the value panel of every bone track"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { ExpandAllTracks(); }), FCanExecuteAction::CreateLambda([this]() { return Asset->Timeline.Tracks.Num() > 0; })));
	Menu.AddMenuEntry(LOCTEXT("CollapseAllTracks", "Collapse All"), LOCTEXT("CollapseAllTip", "Close the value panel of every bone track"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { CollapseAllTracks(); }), FCanExecuteAction::CreateLambda([this]() { return ExpandedCount() > 0; })));
	Menu.EndSection();
	Menu.BeginSection("KeyClipboard", LOCTEXT("KeysSection2", "Keys"));
	Menu.AddMenuEntry(LOCTEXT("CopyKeysMenu", "Copy Keys (Ctrl+C)"), LOCTEXT("CopyKeysTip", "Copy the selected keys (drag a rectangle over the lanes to select several)"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { CopySelectedKeys(); }), FCanExecuteAction::CreateLambda([this]() { return HasSelectedKeys(); })));
	Menu.AddMenuEntry(LOCTEXT("DuplicateKeysMenu", "Copy Keys and Paste at Playhead"), LOCTEXT("DuplicateKeysTip", "Duplicate the selected keys to the playhead in one step"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { DuplicateKeysToPlayhead(); }), FCanExecuteAction::CreateLambda([this]() { return CanDuplicateKeysToPlayhead(); })));
	Menu.AddMenuEntry(LOCTEXT("PasteKeysMenu2", "Paste Keys (Ctrl+V)"), LOCTEXT("PasteKeysTip2", "Paste the copied keys onto their bone tracks at the playhead"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { PasteKeys(); }), FCanExecuteAction::CreateLambda([this]() { return CanPasteKeys(); })));
	Menu.EndSection();

	if (Track.Kind == EAnimStitchTrackKind::Bone)
	{
		// Per-channel commands: key one channel at the playhead, lock or unlock its lane.
		Menu.BeginSection("TrackChannels", LOCTEXT("ChannelsSection", "Channels"));
		static const uint8 Bits[3] = { AnimStitchChannel::Translation, AnimStitchChannel::Rotation, AnimStitchChannel::Scale };
		const FText Names[3] = { LOCTEXT("ChannelTranslation", "Translation"), LOCTEXT("ChannelRotation", "Rotation"), LOCTEXT("ChannelScale", "Scale") };
		for (int32 c = 0; c < 3; ++c)
		{
			const uint8 Bit = Bits[c];
			if (!(FAnimStitchManualEdits::GetApplicableChannels(Track) & Bit)) { continue; }
			Menu.AddMenuEntry(FText::Format(LOCTEXT("AddChannelKey", "Add {0} Key at Playhead"), Names[c]), LOCTEXT("AddChannelKeyTip", "Key just this channel at the playhead (the other channels get no key)"), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, Bit]()
				{
					const FScopedTransaction Tx(LOCTEXT("AddChannelKeyTx", "Add Channel Key"));
					Asset->SetTrackChannelKey(TrackIndex, MakeKeyAtPlayhead(TrackIndex), Bit);
				}), FCanExecuteAction::CreateLambda([this, TrackIndex, Bit]() { return IsChannelEditable(TrackIndex, Bit); })));
			const bool bLockedNow = (Track.UnlockedChannels & Bit) == 0;
			Menu.AddMenuEntry(FText::Format(bLockedNow ? LOCTEXT("UnlockChannel", "Unlock {0} Lane") : LOCTEXT("LockChannel", "Lock {0} Lane"), Names[c]), LOCTEXT("LockChannelTip", "A locked lane keeps its keys visible but they cannot be moved or edited"), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, Bit, bLockedNow]()
				{
					const FScopedTransaction Tx(LOCTEXT("ToggleChannelLockTx", "Lock / Unlock Channel"));
					Asset->SetTrackChannelUnlocked(TrackIndex, Bit, bLockedNow);
				})));
		}
		Menu.EndSection();
	}
	if (Track.Keys.Num() > 0)
	{
		Menu.AddMenuEntry(LOCTEXT("KeyAtPlayhead", "Add Key at Playhead"), LOCTEXT("KeyAtPlayheadTip", "Key the current interpolated value"), FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, TrackIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("AddKeyTx", "Add Key"));
				Asset->SetTrackChannelKey(TrackIndex, MakeKeyAtPlayhead(TrackIndex), FAnimStitchManualEdits::GetEditableChannels(Asset->Timeline.Tracks[TrackIndex]));
				})));
		}
		if (bIK && Track.Keys.Num() > 0)
	{
		Menu.AddMenuEntry(LOCTEXT("ReleaseIK", "Release IK Here"), LOCTEXT("ReleaseIKTip", "Hold the target here, then ease the IK off over the next 0.1 s so the limb returns to the animation"), FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, TrackIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("ReleaseIKTx", "Release IK"));
				// Hold the current target here, then ease the weight to 0 over the next 0.1 s.
				FAnimStitchTrackKey Hold = FAnimStitchManualEdits::Sample(Asset->Timeline.Tracks[TrackIndex], Playhead);
				Hold.Time = Playhead;
				Asset->SetTrackKey(TrackIndex, Hold);
				FAnimStitchTrackKey Off = Hold;
				Off.Time = Playhead + 0.1;
				Off.Weight = 0.f;
				Asset->SetTrackKey(TrackIndex, Off);
			})));
	}
	if (KeyIndex != INDEX_NONE)
	{
		Menu.AddSeparator();
		if (bIK)
		{
			Menu.AddMenuEntry(LOCTEXT("KeyWeight1", "Key Weight 1 (IK on, plants on the target)"), LOCTEXT("KeyWeight1Tip", "Sets this key's IK weight to 1 so the limb sits on the IK target"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, KeyIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("KeyWeightTx", "Set Key Weight"));
				Asset->SetTrackKeyWeight(TrackIndex, KeyIndex, 1.f);
			})));
			Menu.AddMenuEntry(LOCTEXT("KeyWeight0", "Key Weight 0 (release)"), LOCTEXT("KeyWeight0Tip", "Sets this key's IK weight to 0 so the limb returns to the animation"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, KeyIndex]()
			{
				const FScopedTransaction Tx(LOCTEXT("KeyWeightTx", "Set Key Weight"));
				Asset->SetTrackKeyWeight(TrackIndex, KeyIndex, 0.f);
			})));
		}
		Menu.AddMenuEntry(LOCTEXT("DeleteKey", "Delete Key"), LOCTEXT("DeleteKeyTip", "Delete the key (on a channel lane: just that channel's key; a locked channel is left alone)"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex, KeyIndex, Channel]()
		{
			const FScopedTransaction Tx(LOCTEXT("DeleteKeyTx", "Delete Key"));
			Asset->RemoveTrackChannelKey(TrackIndex, KeyIndex, Channel ? Channel : AnimStitchChannel::All);
			SelectedKey = INDEX_NONE;
			SelectedChannel = 0;
		})));
	}
	Menu.AddSeparator();
	Menu.AddMenuEntry(LOCTEXT("RemoveTrack", "Remove Track"), LOCTEXT("RemoveTrackTip", "Remove this lane and all its keys"), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, TrackIndex]()
	{
		const FScopedTransaction Tx(LOCTEXT("RemoveTrackTx", "Remove Track"));
		Asset->RemoveTrack(TrackIndex);
		SelectedTrack = INDEX_NONE;
		SelectedKey = INDEX_NONE;
	})));
	const FWidgetPath Path = Event.GetEventPath() != nullptr ? *Event.GetEventPath() : FWidgetPath();
	FSlateApplication::Get().PushMenu(AsShared(), Path, Menu.MakeWidget(), Event.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

#undef LOCTEXT_NAMESPACE



