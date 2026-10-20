#include "AnimStitchEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"
#include "SAnimStitchPanel.h"
#include "AnimStitchContentBrowserBadge.h"
#include "Editor.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

static const FName AnimStitchTabName("AnimStitch");

TWeakPtr<SAnimStitchPanel> FAnimStitchEditorModule::ActivePanel;
TWeakPtr<SDockTab> FAnimStitchEditorModule::ActiveTab;
static TStrongObjectPtr<UAnimStitchTimelineAsset> GCarryOverAsset;

void FAnimStitchEditorModule::StartupModule()
{
	FAnimStitchContentBrowserBadge::Register(); // the 'On the AnimStitch timeline' badge in the Content Browser
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AnimStitchTabName, FOnSpawnTab::CreateStatic(&FAnimStitchEditorModule::SpawnTab))
		.SetDisplayName(LOCTEXT("TabTitle", "AnimStitch"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Stitch animation clips together on a timeline and bake them to one sequence."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.AssetActions.CreateAnimAsset"));
}

void FAnimStitchEditorModule::ShutdownModule()
{
	FAnimStitchContentBrowserBadge::Unregister();
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnimStitchTabName);
	}
}

TSharedRef<SDockTab> FAnimStitchEditorModule::SpawnTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SAnimStitchPanel> Panel = SNew(SAnimStitchPanel);
	TWeakPtr<SAnimStitchPanel> WeakPanel = Panel;
	TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab)
	.Label_Lambda([WeakPanel]() { const TSharedPtr<SAnimStitchPanel> Pinned = WeakPanel.Pin(); return Pinned.IsValid() ? Pinned->GetTabTitle() : LOCTEXT("TabTitle", "AnimStitch"); })
	[
		Panel
	];
	// The editor sections (toolbar, viewport, timeline, bottom bar, side panel) live in their own dockable tabs inside this one.
	Panel->InitializeDocking(Tab);
	if (UAnimStitchTimelineAsset* Carried = TakeCarryOverAsset())
	{
		Panel->SetAsset(Carried);
	}
	ActivePanel = Panel;
	ActiveTab = Tab;
	return Tab;
}

void FAnimStitchEditorModule::RequestLayoutReset(UAnimStitchTimelineAsset* CarryOverAsset)
{
	GCarryOverAsset.Reset(CarryOverAsset);
	if (!GEditor) { return; }
	// Close and re-open from the next ticks (never from inside the panel's own menu callback).
	GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateLambda([]()
	{
		if (const TSharedPtr<SDockTab> Tab = ActiveTab.Pin()) { Tab->RequestCloseTab(); }
		GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateLambda([]()
		{
			SAnimStitchPanel::bSkipLayoutPersist = false;
			FGlobalTabmanager::Get()->TryInvokeTab(AnimStitchTabName);
		}));
	}));
}

UAnimStitchTimelineAsset* FAnimStitchEditorModule::TakeCarryOverAsset()
{
	UAnimStitchTimelineAsset* Asset = GCarryOverAsset.Get();
	GCarryOverAsset.Reset();
	return Asset;
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
