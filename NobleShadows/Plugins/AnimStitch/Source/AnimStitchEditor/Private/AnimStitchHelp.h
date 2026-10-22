#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

class FMenuBuilder;

/** Pages of the online documentation. The base URL and the slug of every page live in UAnimStitchSettings (one place). */
enum class EAnimStitchHelpPage : uint8
{
	Overview,
	QuickStart,
	Timeline,
	Baking,
	BatchBaking,
	FootLock,
	Corrections,
};

/** Help: opens the online docs in the browser (deep-linked by context) and shows the built-in Quick Start. No embedded viewer, no bundled manual. */
class FAnimStitchHelp
{
public:
	static FString GetDocsUrl(EAnimStitchHelpPage Page);
	static void OpenDocs(EAnimStitchHelpPage Page);

	/** A small "?" button that opens the given docs page. */
	static TSharedRef<SWidget> MakeHelpButton(EAnimStitchHelpPage Page, const FText& Tip);

	/** Entries of the Help menu (Quick Start + the docs pages). */
	static void FillHelpMenu(FMenuBuilder& Menu);

	/** The built-in Quick Start window (5-step happy path + the shortcut cheat sheet). A second call brings the open one to the front. */
	static void ShowQuickStart();
};
