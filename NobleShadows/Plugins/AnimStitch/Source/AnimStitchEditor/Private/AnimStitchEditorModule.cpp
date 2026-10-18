#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"
#include "SAnimStitchPanel.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

static const FName AnimStitchTabName("AnimStitch");

class FAnimStitchEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AnimStitchTabName, FOnSpawnTab::CreateStatic(&FAnimStitchEditorModule::SpawnTab))
			.SetDisplayName(LOCTEXT("TabTitle", "AnimStitch"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Stitch animation clips together on a timeline and bake them to one sequence."))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.AssetActions.CreateAnimAsset"));
	}

	virtual void ShutdownModule() override
	{
		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnimStitchTabName);
		}
	}

private:
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SAnimStitchPanel)
		];
	}
};

IMPLEMENT_MODULE(FAnimStitchEditorModule, AnimStitchEditor)

#undef LOCTEXT_NAMESPACE
