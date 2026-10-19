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

private:
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);
	static TWeakPtr<SAnimStitchPanel> ActivePanel;
};

