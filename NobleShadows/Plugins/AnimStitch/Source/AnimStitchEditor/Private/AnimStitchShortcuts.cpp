#include "AnimStitchShortcuts.h"
#include "Framework/Commands/InputChord.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

const TArray<FAnimStitchShortcutDef>& FAnimStitchShortcuts::All()
{
	// Built on first use (EKeys must exist). The order is the order of the Quick Start cheat sheet.
	static const TArray<FAnimStitchShortcutDef> Table = []()
	{
		TArray<FAnimStitchShortcutDef> T;
		auto Add = [&T](EAnimStitchShortcut Id, FKey Key, FKey Secondary, bool bCtrl, bool bShift, bool bAlt, const FText& Action, const FText& Where)
		{
			FAnimStitchShortcutDef D;
			D.Id = Id; D.Key = Key; D.SecondaryKey = Secondary; D.bCtrl = bCtrl; D.bShift = bShift; D.bAlt = bAlt; D.Action = Action; D.Where = Where;
			T.Add(D);
		};
		Add(EAnimStitchShortcut::AddKey, EKeys::A, EKeys::Enter, false, false, false, LOCTEXT("ScAddKey", "Add a key at the playhead"), LOCTEXT("ScAddKeyWhere", "Timeline: keys the selected bone track(s)"));
		Add(EAnimStitchShortcut::ZoomToFit, EKeys::Enter, EKeys::Invalid, false, false, false, LOCTEXT("ScZoomFit", "Zoom to Fit"), LOCTEXT("ScZoomFitWhere", "Timeline: when no track is selected"));
		Add(EAnimStitchShortcut::PlayPause, EKeys::SpaceBar, EKeys::Invalid, false, false, false, LOCTEXT("ScPlay", "Play / Pause"), LOCTEXT("ScPlayWhere", "Anywhere in the tool"));
		Add(EAnimStitchShortcut::Save, EKeys::S, EKeys::Invalid, true, false, false, LOCTEXT("ScSave", "Save the timeline"), LOCTEXT("ScSaveWhere", "Anywhere in the tool"));
		Add(EAnimStitchShortcut::ToggleHelpers, EKeys::G, EKeys::Invalid, false, false, false, LOCTEXT("ScHelpers", "Show / hide helpers (controls, bones, floor)"), LOCTEXT("ScHelpersWhere", "Viewport"));
		Add(EAnimStitchShortcut::ToggleGrid, EKeys::G, EKeys::Invalid, false, true, true, LOCTEXT("ScGrid", "Show / hide the ground grid"), LOCTEXT("ScGridWhere", "Anywhere in the tool"));
		Add(EAnimStitchShortcut::GizmoSpace, EKeys::T, EKeys::Invalid, false, false, false, LOCTEXT("ScSpace", "Gizmo space: World / Local"), LOCTEXT("ScSpaceWhere", "Viewport (remembered per tool)"));
		Add(EAnimStitchShortcut::GizmoMove, EKeys::W, EKeys::Invalid, false, false, false, LOCTEXT("ScMove", "Move gizmo"), LOCTEXT("ScMoveWhere", "Viewport"));
		Add(EAnimStitchShortcut::GizmoRotate, EKeys::E, EKeys::Invalid, false, false, false, LOCTEXT("ScRotate", "Rotate gizmo"), LOCTEXT("ScRotateWhere", "Viewport"));
		Add(EAnimStitchShortcut::GizmoScale, EKeys::R, EKeys::Invalid, false, false, false, LOCTEXT("ScScale", "Scale gizmo"), LOCTEXT("ScScaleWhere", "Viewport"));
		Add(EAnimStitchShortcut::FrameSelection, EKeys::F, EKeys::Invalid, false, false, false, LOCTEXT("ScFrame", "Frame the selection (the character when nothing is selected)"), LOCTEXT("ScFrameWhere", "Viewport"));
		Add(EAnimStitchShortcut::CopyKeys, EKeys::C, EKeys::Invalid, true, false, false, LOCTEXT("ScCopy", "Copy the selected keys"), LOCTEXT("ScCopyWhere", "Timeline"));
		Add(EAnimStitchShortcut::PasteKeys, EKeys::V, EKeys::Invalid, true, false, false, LOCTEXT("ScPaste", "Paste keys at the playhead"), LOCTEXT("ScPasteWhere", "Timeline"));
		Add(EAnimStitchShortcut::DuplicateClip, EKeys::D, EKeys::Invalid, true, false, false, LOCTEXT("ScDuplicate", "Duplicate the selected clip"), LOCTEXT("ScDuplicateWhere", "Timeline"));
		Add(EAnimStitchShortcut::RateStretch, EKeys::Invalid, EKeys::Invalid, false, false, true, LOCTEXT("ScRate", "Stretch a clip (change its play rate)"), LOCTEXT("ScRateWhere", "Timeline: drag a clip edge"));
		return T;
	}();
	return Table;
}

const FAnimStitchShortcutDef& FAnimStitchShortcuts::Get(EAnimStitchShortcut Id)
{
	for (const FAnimStitchShortcutDef& D : All())
	{
		if (D.Id == Id) { return D; }
	}
	checkNoEntry();
	return All()[0];
}

bool FAnimStitchShortcuts::Matches(EAnimStitchShortcut Id, const FKey& Key, bool bCtrl, bool bShift, bool bAlt)
{
	const FAnimStitchShortcutDef& D = Get(Id);
	if (!Key.IsValid() || (Key != D.Key && Key != D.SecondaryKey)) { return false; }
	return D.bCtrl == bCtrl && D.bShift == bShift && D.bAlt == bAlt;
}

bool FAnimStitchShortcuts::Matches(EAnimStitchShortcut Id, const FKeyEvent& Event)
{
	return Matches(Id, Event.GetKey(), Event.IsControlDown(), Event.IsShiftDown(), Event.IsAltDown());
}

bool FAnimStitchShortcuts::ModifiersMatch(EAnimStitchShortcut Id, const FInputEvent& Event)
{
	const FAnimStitchShortcutDef& D = Get(Id);
	return D.bCtrl == Event.IsControlDown() && D.bShift == Event.IsShiftDown() && D.bAlt == Event.IsAltDown();
}

FText FAnimStitchShortcuts::GetKeysText(EAnimStitchShortcut Id)
{
	const FAnimStitchShortcutDef& D = Get(Id);
	auto Chord = [&D](const FKey& Key) { return FInputChord(Key, EModifierKey::FromBools(D.bCtrl, D.bAlt, D.bShift, false)).GetInputText(); };
	if (!D.Key.IsValid())
	{
		// Modifier-only gesture (the key is held while dragging).
		return FText::Format(LOCTEXT("ModifierDrag", "{0} + drag"), D.bAlt ? LOCTEXT("ModAlt", "Alt") : D.bCtrl ? LOCTEXT("ModCtrl", "Ctrl") : LOCTEXT("ModShift", "Shift"));
	}
	if (D.SecondaryKey.IsValid())
	{
		return FText::Format(LOCTEXT("TwoKeys", "{0} / {1}"), Chord(D.Key), Chord(D.SecondaryKey));
	}
	return Chord(D.Key);
}

#undef LOCTEXT_NAMESPACE
