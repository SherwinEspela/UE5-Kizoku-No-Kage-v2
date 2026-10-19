#include "AnimStitchEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"
#include "SAnimStitchPanel.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

static const FName AnimStitchTabName("AnimStitch");

TWeakPtr<SAnimStitchPanel> FAnimStitchEditorModule::ActivePanel;

void FAnimStitchEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AnimStitchTabName, FOnSpawnTab::CreateStatic(&FAnimStitchEditorModule::SpawnTab))
		.SetDisplayName(LOCTEXT("TabTitle", "AnimStitch"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Stitch animation clips together on a timeline and bake them to one sequence."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.AssetActions.CreateAnimAsset"));
}

void FAnimStitchEditorModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnimStitchTabName);
	}
}

TSharedRef<SDockTab> FAnimStitchEditorModule::SpawnTab(const FSpawnTabArgs& Args)
{
	TSharedPtr<SAnimStitchPanel> Panel;
	TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab)
	[
		SAssignNew(Panel, SAnimStitchPanel)
	];
	ActivePanel = Panel;
	return Tab;
}

void FAnimStitchEditorModule::OpenTimelineAsset(UAnimStitchTimelineAsset* Timeline)
{
	FGlobalTabmanager::Get()->TryInvokeTab(AnimStitchTabName);
	if (const TSharedPtr<SAnimStitchPanel> Panel = ActivePanel.Pin())
	{
		Panel->SetAsset(Timeline);
	}
}

IMPLEMENT_MODULE(FAnimStitchEditorModule, AnimStitchEditor)

#undef LOCTEXT_NAMESPACE
