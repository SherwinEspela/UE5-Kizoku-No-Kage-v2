#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

struct FInputEvent;
struct FKeyEvent;

/** Every keyboard / modifier shortcut of the tool. The input handlers AND the Quick Start cheat sheet read this one table, so they cannot drift apart. */
enum class EAnimStitchShortcut : uint8
{
	AddKey,
	ZoomToFit,
	PlayPause,
	Save,
	ToggleHelpers,
	ToggleGrid,
	GizmoSpace,
	GizmoMove,
	GizmoRotate,
	GizmoScale,
	FrameSelection,
	CopyKeys,
	PasteKeys,
	DuplicateClip,
	RateStretch,
};

struct FAnimStitchShortcutDef
{
	EAnimStitchShortcut Id;
	/** Primary key; the secondary key (if valid) triggers the same action. Invalid for modifier-only gestures (Alt + drag). */
	FKey Key;
	FKey SecondaryKey;
	bool bCtrl = false;
	bool bShift = false;
	bool bAlt = false;
	/** What it does, and where it works (the cheat sheet shows both). */
	FText Action;
	FText Where;
};

class FAnimStitchShortcuts
{
public:
	static const TArray<FAnimStitchShortcutDef>& All();
	static const FAnimStitchShortcutDef& Get(EAnimStitchShortcut Id);

	/** True when the key event is this shortcut (exact modifiers). */
	static bool Matches(EAnimStitchShortcut Id, const FKeyEvent& Event);
	/** Same for callers that only have a key and the modifier state (viewport client). */
	static bool Matches(EAnimStitchShortcut Id, const FKey& Key, bool bCtrl, bool bShift, bool bAlt);
	/** For mouse gestures: the modifier keys of the event are exactly the shortcut's. */
	static bool ModifiersMatch(EAnimStitchShortcut Id, const FInputEvent& Event);

	/** "A / Enter", "Ctrl+S", "Alt + drag" ... built from the table. */
	static FText GetKeysText(EAnimStitchShortcut Id);
};
