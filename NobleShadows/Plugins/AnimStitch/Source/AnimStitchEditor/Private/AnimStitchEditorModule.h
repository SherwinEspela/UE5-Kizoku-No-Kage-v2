#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class SAnimStitchPanel;
class SDockTab;
class FSpawnTabArgs;
class UAnimStitchTimelineAsset;

class FAnimStitchEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Opens (or focuses) the AnimStitch tab and loads the asset into it. */
	static void OpenTimelineAsset(UAnimStitchTimelineAsset* Timeline);

	/** Closes and re-opens the AnimStitch tab so it comes back in the default layout; the timeline in it is kept. */
	static void RequestLayoutReset(UAnimStitchTimelineAsset* CarryOverAsset);
	/** The timeline carried over a layout reset (null otherwise); taken once by the new panel. */
	static UAnimStitchTimelineAsset* TakeCarryOverAsset();

private:
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);
	static TWeakPtr<SAnimStitchPanel> ActivePanel;
	static TWeakPtr<SDockTab> ActiveTab;
};

