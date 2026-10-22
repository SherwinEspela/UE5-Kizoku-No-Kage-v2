#include "AnimStitchHelp.h"
#include "AnimStitchShortcuts.h"
#include "AnimStitchSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformProcess.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

FString FAnimStitchHelp::GetDocsUrl(EAnimStitchHelpPage Page)
{
	const UAnimStitchSettings* Settings = GetDefault<UAnimStitchSettings>();
	const FAnimStitchDocsPages& Pages = Settings->DocsPages;
	const FString* Slug = &Pages.Overview;
	switch (Page)
	{
	case EAnimStitchHelpPage::QuickStart: Slug = &Pages.QuickStart; break;
	case EAnimStitchHelpPage::Timeline: Slug = &Pages.Timeline; break;
	case EAnimStitchHelpPage::Baking: Slug = &Pages.Baking; break;
	case EAnimStitchHelpPage::BatchBaking: Slug = &Pages.BatchBaking; break;
	case EAnimStitchHelpPage::FootLock: Slug = &Pages.FootLock; break;
	case EAnimStitchHelpPage::Corrections: Slug = &Pages.Corrections; break;
	default: break;
	}
	FString Base = Settings->DocsBaseUrl;
	while (Base.EndsWith(TEXT("/"))) { Base.LeftChopInline(1); }
	return Slug->IsEmpty() ? Base : Base + TEXT("/") + *Slug;
}

void FAnimStitchHelp::OpenDocs(EAnimStitchHelpPage Page)
{
	const FString Url = GetDocsUrl(Page);
	FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
}

TSharedRef<SWidget> FAnimStitchHelp::MakeHelpButton(EAnimStitchHelpPage Page, const FText& Tip)
{
	return SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(4.f))
		.ToolTipText(Tip)
		.OnClicked_Lambda([Page]() { OpenDocs(Page); return FReply::Handled(); })
		[
			SNew(SImage).Image(FAppStyle::GetBrush("Icons.Help")).ColorAndOpacity(FSlateColor::UseForeground())
		];
}

void FAnimStitchHelp::FillHelpMenu(FMenuBuilder& Menu)
{
	Menu.BeginSection("HelpQuick", LOCTEXT("HelpQuickSection", "Getting started"));
	Menu.AddMenuEntry(LOCTEXT("HelpQuickStart", "Quick Start..."), LOCTEXT("HelpQuickStartTip", "The 5-step happy path and the keyboard shortcuts, in a small window inside the tool"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FAnimStitchHelp::ShowQuickStart)));
	Menu.EndSection();
	Menu.BeginSection("HelpDocs", LOCTEXT("HelpDocsSection", "Online documentation (opens your browser)"));
	auto Entry = [&Menu](const FText& Label, const FText& Tip, EAnimStitchHelpPage Page)
	{
		Menu.AddMenuEntry(Label, Tip, FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([Page]() { FAnimStitchHelp::OpenDocs(Page); })));
	};
	Entry(LOCTEXT("HelpOverview", "Documentation Home"), LOCTEXT("HelpOverviewTip", "The AnimStitch documentation start page"), EAnimStitchHelpPage::Overview);
	Entry(LOCTEXT("HelpTimeline", "Timeline"), LOCTEXT("HelpTimelineTip", "Clips, overlaps and crossfades, trimming, play rate, range flags, zoom"), EAnimStitchHelpPage::Timeline);
	Entry(LOCTEXT("HelpFootLock", "Foot Lock"), LOCTEXT("HelpFootLockTip", "Plant detection, manual locks, pelvis follow, predict feet"), EAnimStitchHelpPage::FootLock);
	Entry(LOCTEXT("HelpCorrections", "Manual Corrections"), LOCTEXT("HelpCorrectionsTip", "Bone, FK and IK tracks, channel lanes and locks, absolute tracks, root offset"), EAnimStitchHelpPage::Corrections);
	Entry(LOCTEXT("HelpBaking", "Baking"), LOCTEXT("HelpBakingTip", "Bake options, partial range, the Baked Assets tab"), EAnimStitchHelpPage::Baking);
	Entry(LOCTEXT("HelpBatch", "Batch Baking"), LOCTEXT("HelpBatchTip", "Use a timeline as a template over many source clips"), EAnimStitchHelpPage::BatchBaking);
	Menu.EndSection();
}

namespace
{
	TWeakPtr<SWindow> QuickStartWindow;

	/** Undo / Redo belong to the editor (not to this tool), so their keys are read from the editor's own commands. */
	FText EditorChordText(const TSharedPtr<FUICommandInfo>& Command, const TCHAR* Fallback)
	{
		if (Command.IsValid())
		{
			const TSharedRef<const FInputChord> Chord = Command->GetFirstValidChord();
			if (Chord->IsValidChord()) { return Chord->GetInputText(); }
		}
		return FText::FromString(Fallback);
	}

	TSharedRef<SWidget> ShortcutRow(const FText& Keys, const FText& Action, const FText& Where)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 2.f, 10.f, 2.f)
			[
				SNew(SBox).WidthOverride(130.f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(6.f, 2.f))
					[
						SNew(STextBlock).Text(Keys).Font(FAppStyle::GetFontStyle("BoldFont"))
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Action).AutoWrapText(true)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Where).ColorAndOpacity(FSlateColor::UseSubduedForeground()).AutoWrapText(true)]
			];
	}

	TSharedRef<SWidget> StepRow(int32 Number, const FText& Title, const FText& Body)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 2.f, 10.f, 2.f)
			[
				SNew(SBox).WidthOverride(26.f)
				[
					SNew(STextBlock).Text(FText::AsNumber(Number)).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 2.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle("BoldFont"))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Body).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
			];
	}
}

void FAnimStitchHelp::ShowQuickStart()
{
	if (const TSharedPtr<SWindow> Existing = QuickStartWindow.Pin())
	{
		Existing->BringToFront();
		return;
	}

	TSharedRef<SVerticalBox> Shortcuts = SNew(SVerticalBox);
	for (const FAnimStitchShortcutDef& Def : FAnimStitchShortcuts::All())
	{
		Shortcuts->AddSlot().AutoHeight()[ShortcutRow(FAnimStitchShortcuts::GetKeysText(Def.Id), Def.Action, Def.Where)];
	}
	// The editor's own undo / redo keys.
	const bool bHaveCommands = FGenericCommands::IsRegistered();
	Shortcuts->AddSlot().AutoHeight()[ShortcutRow(
		EditorChordText(bHaveCommands ? FGenericCommands::Get().Undo : nullptr, TEXT("Ctrl+Z")),
		LOCTEXT("ScUndo", "Undo (Redo: the editor's Redo key)"), LOCTEXT("ScUndoWhere", "Every edit in the tool is one undo step"))];

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("QuickStartTitle", "AnimStitch - Quick Start"))
		.ClientSize(FVector2D(560.0, 720.0))
		.SupportsMinimize(false)
		.SupportsMaximize(false);
	Window->SetContent(
		SNew(SBorder).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[SNew(STextBlock).Text(LOCTEXT("QuickStartHeading", "The happy path")).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
					+ SVerticalBox::Slot().AutoHeight()[StepRow(1, LOCTEXT("Step1", "Add clips"), LOCTEXT("Step1Body", "Drag animation sequences from the Content Browser tab onto the timeline. The first clip picks the preview mesh and the skeleton."))]
					+ SVerticalBox::Slot().AutoHeight()[StepRow(2, LOCTEXT("Step2", "Overlap / crossfade"), LOCTEXT("Step2Body", "Drag the strip where two clips meet (or drag a clip over its neighbour) to set the crossfade. Pull clips apart to open a gap that crossfades too. Ease In/Out or Linear sets the blend curve."))]
					+ SVerticalBox::Slot().AutoHeight()[StepRow(3, LOCTEXT("Step3", "Foot lock"), LOCTEXT("Step3Body", "Switch Foot Lock on so planted feet stop sliding through the blend. Click or drag the L / R plant lanes under the ruler to lock or unlock a foot by hand."))]
					+ SVerticalBox::Slot().AutoHeight()[StepRow(4, LOCTEXT("Step4", "Correct"), LOCTEXT("Step4Body", "Add Core Bone or Add Limb tracks and move the controls in the viewport, or type values in Details. Expand a bone track for separate Translation, Rotation and Scale key lanes (Translation and Scale start locked)."))]
					+ SVerticalBox::Slot().AutoHeight()[StepRow(5, LOCTEXT("Step5", "Bake"), LOCTEXT("Step5Body", "Press Bake... to write one animation sequence. Only the region between the range flags is baked; the result appears in the Baked Assets tab."))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 6.f)[SNew(STextBlock).Text(LOCTEXT("ShortcutsHeading", "Shortcuts")).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
					+ SVerticalBox::Slot().AutoHeight()[Shortcuts]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f).HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SButton).Text(LOCTEXT("QuickStartDocs", "Open Documentation"))
					.ToolTipText(LOCTEXT("QuickStartDocsTip", "Opens the online documentation in your browser"))
					.OnClicked_Lambda([]() { FAnimStitchHelp::OpenDocs(EAnimStitchHelpPage::QuickStart); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("QuickStartClose", "Close"))
					.ToolTipText(LOCTEXT("QuickStartCloseTip", "Close this window"))
					.OnClicked_Lambda([WeakWindow = TWeakPtr<SWindow>(Window)]() { if (TSharedPtr<SWindow> W = WeakWindow.Pin()) { W->RequestDestroyWindow(); } return FReply::Handled(); })
				]
			]
		]);
	QuickStartWindow = Window;
	FSlateApplication::Get().AddWindow(Window);
}

#undef LOCTEXT_NAMESPACE
