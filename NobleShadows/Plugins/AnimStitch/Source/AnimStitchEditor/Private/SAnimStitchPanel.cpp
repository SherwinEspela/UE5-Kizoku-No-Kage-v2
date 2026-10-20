#include "SAnimStitchPanel.h"
#include "SAnimStitchTimeline.h"
#include "SAnimStitchKeyPanel.h"
#include "SAnimStitchTransportBar.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "SAnimStitchControlProperties.h"
#include "AnimStitchContentBrowserBadge.h"
#include "IContentBrowserSingleton.h"
#include "SAnimStitchAttachedProps.h"
#include "AnimStitchManualEdits.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Docking/LayoutService.h"
#include "Widgets/Docking/SDockTab.h"
#include "AnimStitchBaker.h"
#include "AnimStitchEditorModule.h"
#include "AssetThumbnail.h"
#include "Editor/Transactor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SOverlay.h"
#include "AnimStitchPreview.h"
#include "AnimStitchSettings.h"
#include "Engine/SkeletalMesh.h"
#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "AnimStitchAssetTypes.h"
#include "SAnimStitchHoverPreview.h"
#include "AnimStitchFootLock.h"
#include "Widgets/SToolTip.h"
#include "SAnimStitchBatchDialog.h"
#include "SAnimStitchBakeDialog.h"
#include "SAnimStitchFootLockSettings.h"
#include "SAnimStitchBoneDetails.h"
#include "SAnimStitchAttachDialog.h"
#include "Widgets/Input/SSearchableComboBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Images/SImage.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SComboButton.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchPanel::Construct(const FArguments& InArgs)
{
	Asset = NewObject<UAnimStitchTimelineAsset>(GetTransientPackage(), NAME_None, RF_Transactional);
	Preview = MakeShared<FAnimStitchPreview>();
	Preview->OnBoneSelectionChanged.AddSP(this, &SAnimStitchPanel::OnPreviewBoneSelected);
	Preview->SetAsset(Asset);

	auto CurveButton = [this](EAnimStitchBlendCurve Curve, const FText& Label)
	{
		return SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.IsEnabled(this, &SAnimStitchPanel::IsCurveEnabled)
			.IsChecked(this, &SAnimStitchPanel::GetCurveState, Curve)
			.OnCheckStateChanged_Lambda([this, Curve](ECheckBoxState) { Timeline->SetSelectedBlendCurve(Curve); })
			[
				SNew(STextBlock).Text(Label).Margin(FMargin(8.f, 2.f))
			];
	};

	auto ToggleButton = [](const FText& Label, const FText& Tip, TFunction<bool()> Get, TFunction<void(bool)> Set)
	{
		return SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(Tip)
			.IsChecked_Lambda([Get]() { return Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([Set](ECheckBoxState State) { Set(State == ECheckBoxState::Checked); })
			[
				SNew(STextBlock).Text(Label).Margin(FMargin(8.f, 2.f))
			];
	};

	auto IconButton = [](TAttribute<const FSlateBrush*> Brush, const FText& Tip, TFunction<void()> OnClick)
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(Tip)
			.ContentPadding(FMargin(4.f))
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				SNew(SImage).Image(Brush).ColorAndOpacity(FSlateColor::UseForeground())
			];
	};

	auto RightTabButton = [this](const FText& Label, int32 TabIndex)
	{
		return SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.IsChecked_Lambda([this, TabIndex]() { return RightTab == TabIndex ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, TabIndex](ECheckBoxState State) { if (State == ECheckBoxState::Checked) { RightTab = TabIndex; if (TabIndex == 2) { RebuildBakedList(); } } })
			[
				SNew(STextBlock).Text(Label).Margin(FMargin(10.f, 3.f))
			];
	};

		auto AssetButton =[](const FText& Label, const FText& Tip, TFunction<void()> OnClick)
	{
		return SNew(SButton)
			.Text(Label)
			.ToolTipText(Tip)
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); });
	};

	// Icon helpers for the edit buttons (icons with tooltips, no text buttons).
	auto Glyph = [](const char* Brush) -> TSharedRef<SWidget>
	{
		return SNew(SBox).WidthOverride(14.f).HeightOverride(14.f)[SNew(SImage).Image(FAppStyle::GetBrush(Brush)).ColorAndOpacity(FSlateColor::UseForeground())];
	};
	auto Bar = []() -> TSharedRef<SWidget>
	{
		return SNew(SBox).WidthOverride(2.f).HeightOverride(12.f)[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FSlateColor::UseForeground())];
	};
	auto Icons = [](std::initializer_list<TSharedRef<SWidget>> Parts) -> TSharedRef<SWidget>
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (const TSharedRef<SWidget>& Part : Parts) { Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[Part]; }
		return Row;
	};

	auto TimelineButton = [this](TSharedRef<SWidget> Icon, const FText& Tip, TFunction<void()> OnClick)
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(5.f, 4.f))
			.ToolTipText(Tip)
			.IsEnabled_Lambda([this]() { return Timeline.IsValid() && Timeline->CanEditAtPlayhead(); })
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				Icon
			];
	};
	SAssignNew(Timeline, SAnimStitchTimeline)
		.TimelineAsset(Asset)
		.OnPlayheadChanged(FSimpleDelegate::CreateSP(this, &SAnimStitchPanel::OnTimelineScrubbed))
		.OnWarnings(FAnimStitchWarningsDelegate::CreateSP(this, &SAnimStitchPanel::HandleWarnings));
	Timeline->LiveKeyProvider = [this](int32 Track, FAnimStitchTrackKey& OutKey) { return Preview->GetIKLiveKey(Track, OutKey); };

	// ---- The sections of the editor. Each one becomes a tab of its own, so it can be docked anywhere (see InitializeDocking). ----
	ViewportSection = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeViewportToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(Viewport, SAnimStitchViewport).Preview(Preview)
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(6.f)
					.Visibility(this, &SAnimStitchPanel::GetProblemVisibility)
					[
						SNew(STextBlock).Text(this, &SAnimStitchPanel::GetProblemText).AutoWrapText(true)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					// Shown while a pelvis track has Keep feet planted on.
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(6.f)
					.Visibility_Lambda([this]()
					{
						for (const FAnimStitchTrack& Track : Asset->Timeline.Tracks)
						{
							if (Track.Kind == EAnimStitchTrackKind::Bone && Track.bKeepFeetPlanted && FAnimStitchManualEdits::IsPelvisBone(Track.Bone)) { return EVisibility::HitTestInvisible; }
						}
						return EVisibility::Collapsed;
					})
					[
						SNew(STextBlock).Text(LOCTEXT("KeepFeetStatus", "Keep feet planted: ON - the feet stay on the ground (leg IK) while the pelvis moves"))
					]
				]
			]
		];

	TSharedRef<SVerticalBox> ToolbarRows = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 4.f, 6.f, 0.f)
		[
			// Layout menu: reset the docking layout, bring back closed sections.
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[SNullWidget::NullWidget]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SComboButton).HasDownArrow(true)
				.ToolTipText(LOCTEXT("LayoutTip", "Docking layout of the editor sections"))
				.ButtonContent()[SNew(STextBlock).Text(LOCTEXT("LayoutLabel", "Layout"))]
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget> { return MakeLayoutMenu(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 6.f, 6.f, 2.f)
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				IconButton(TAttribute<const FSlateBrush*>(FAppStyle::GetBrush("Icons.Plus")), LOCTEXT("NewTimelineTip", "New: start a new, empty timeline"), [this]() { NewTimeline(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				IconButton(TAttribute<const FSlateBrush*>(FAppStyle::GetBrush("Icons.FolderOpen")), LOCTEXT("OpenTimelineTip", "Open a saved AnimStitch timeline asset"), [this]() { OpenFromDisk(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				IconButton(TAttribute<const FSlateBrush*>::CreateLambda([this]() { return FAppStyle::GetBrush(IsDirtyState() ? "Icons.SaveChanged" : "AssetEditor.SaveAsset"); }), LOCTEXT("SaveTimelineTip", "Save the timeline as an asset (Ctrl+S). An asterisk on the icon means unsaved changes."), [this]() { Save(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				IconButton(TAttribute<const FSlateBrush*>(FAppStyle::GetBrush("AssetEditor.SaveAssetAs")), LOCTEXT("SaveTimelineAsTip", "Save a copy of the timeline as a new asset"), [this]() { SaveAs(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				AssetButton(LOCTEXT("BakeSingle", "Bake..."), LOCTEXT("BakeSingleTip", "Bake the timeline to a single animation sequence"), [this]() { OpenBake(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 14, 3)
			[
				AssetButton(LOCTEXT("BatchBake", "Batch Bake..."), LOCTEXT("BatchBakeTip", "Use this timeline as a template and bake it over many source clips"), [this]() { OpenBatch(); })
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(0, 0, 14, 3)
			[
				SNew(STextBlock).Text(this, &SAnimStitchPanel::GetAssetLabel)
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				ToggleButton(LOCTEXT("PredictFeet", "Predict Feet (experimental)"),
					LOCTEXT("PredictFeetTip", "EXPERIMENTAL. Makes crossfades plant-aware: plants are detected on each source clip on its own, then inside every crossfade the outgoing clip's planted feet are held and the feet settle onto the incoming clip's stance. The pelvis is never moved. Works with Foot Lock (prediction first, Foot Lock cleans up). Uses the foot bones and thresholds from the Foot Lock settings."),
					[this]() { return Asset->Timeline.bPredictFeet; },
					[this](bool bOn)
					{
						const FScopedTransaction Tx(LOCTEXT("PredictFeetTx", "Toggle Predict Feet"));
						Asset->SetPredictFeet(bOn);
					})
			]
			+ SWrapBox::Slot().Padding(0, 0, 2, 3)
			[
				ToggleButton(LOCTEXT("FootLock", "Foot Lock"),
					LOCTEXT("FootLockToggleTip", "Pin planted feet (and manually locked feet) in the preview and in every bake. Settings are in the dropdown."),
					[this]() { return Asset->Timeline.FootLock.bEnabled; },
					[this](bool bOn)
					{
						FAnimStitchFootLockSettings Settings = Asset->Timeline.FootLock;
						Settings.bEnabled = bOn;
						const FScopedTransaction Tx(LOCTEXT("FootLockToggleTx", "Toggle Foot Lock"));
						Asset->SetFootLockSettings(Settings);
					})
			]
			+ SWrapBox::Slot().Padding(0, 0, 2, 3)
			[
				SNew(SComboButton)
				.HasDownArrow(true)
				.ToolTipText(LOCTEXT("FootLockSettingsTip", "Foot lock settings: foot bones, ground height, plant detection"))
				.ButtonContent()[SNew(STextBlock).Text(LOCTEXT("FootLockSettingsLabel", "Settings"))]
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget>
				{
					return SNew(SBox).MinDesiredWidth(600.f).Padding(10.f)
					[
						SNew(SAnimStitchFootLockSettings).Timeline(Asset)
					];
				})
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 2.f, 6.f, 2.f)
		[
			SNew(SWrapBox).UseAllottedSize(true)
			+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(0, 0, 6, 3)
			[
				SNew(STextBlock).Text(LOCTEXT("PreviewMesh", "Preview mesh:"))
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(0, 0, 12, 3)
			[
				SNew(SBox).MinDesiredWidth(220.f).MaxDesiredWidth(320.f)[SNew(SObjectPropertyEntryBox)
				.AllowedClass(USkeletalMesh::StaticClass())
				.ObjectPath(this, &SAnimStitchPanel::GetMeshPath)
				.OnObjectChanged(this, &SAnimStitchPanel::OnMeshChanged)
				.OnShouldFilterAsset(this, &SAnimStitchPanel::ShouldFilterMesh)
				.AllowClear(true)
				.DisplayThumbnail(false)]
			]
			+ SWrapBox::Slot().Padding(0, 0, 12, 3)
			[
				SNew(SComboButton).HasDownArrow(true)
				.ToolTipText(LOCTEXT("AttachPropsTip", "Attach props (static or skeletal meshes) to a socket or bone of the preview character. Preview only, never baked."))
				.ButtonContent()[SNew(STextBlock).Text(LOCTEXT("AttachProps", "Attach Props"))]
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget> { FMenuBuilder Menu(true, nullptr); FillAttachmentsMenu(Menu); return Menu.MakeWidget(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 4, 3)
			[
				ToggleButton(LOCTEXT("Ghosts", "Ghost Joins"), LOCTEXT("GhostsTip", "Show the last pose of the outgoing clip (orange) and the first pose of the incoming clip (blue) at every join"),
					[this]() { return bShowGhosts; }, [this](bool b) { bShowGhosts = b; Preview->SetShowGhosts(b); })
			]
			+ SWrapBox::Slot()
			[
				ToggleButton(LOCTEXT("RootPath", "Root Path"), LOCTEXT("RootPathTip", "Draw the root bone's path across the timeline on the ground"),
					[this]() { return bShowRootPath; }, [this](bool b) { bShowRootPath = b; Preview->SetShowRootPath(b); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 2.f, 6.f, 4.f)
		[
			SNew(SWrapBox).UseAllottedSize(true)
			// Edit group: Undo, Redo, Duplicate, ...
			+ SWrapBox::Slot().Padding(0, 0, 2, 3)
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(4.f))
				.ToolTipText(LOCTEXT("UndoTip", "Undo (Ctrl+Z)"))
				.IsEnabled_Lambda([]() { return GEditor && GEditor->Trans && GEditor->Trans->CanUndo(); })
				.OnClicked_Lambda([]() { if (GEditor) { GEditor->UndoTransaction(); } return FReply::Handled(); })
				[
					SNew(SImage).Image(FAppStyle::GetBrush("GenericCommands.Undo")).ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SWrapBox::Slot().Padding(0, 0, 8, 3)
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(4.f))
				.ToolTipText(LOCTEXT("RedoTip", "Redo (Ctrl+Y)"))
				.IsEnabled_Lambda([]() { return GEditor && GEditor->Trans && GEditor->Trans->CanRedo(); })
				.OnClicked_Lambda([]() { if (GEditor) { GEditor->RedoTransaction(); } return FReply::Handled(); })
				[
					SNew(SImage).Image(FAppStyle::GetBrush("GenericCommands.Redo")).ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(5.f, 4.f))
				.ToolTipText(LOCTEXT("DuplicateTip", "Duplicate the selected clip (Ctrl+D)"))
				.OnClicked_Lambda([this]() { Timeline->DuplicateSelected(); return FReply::Handled(); })
				[Glyph("GenericCommands.Duplicate")]
			]
			+ SWrapBox::Slot().Padding(0, 0, 18, 3)
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(5.f, 4.f))
				.ToolTipText(LOCTEXT("RemoveTip", "Remove the selected clip (Delete)"))
				.OnClicked_Lambda([this]() { Timeline->DeleteSelected(); return FReply::Handled(); })
				[Glyph("GenericCommands.Delete")]
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				TimelineButton(Glyph("GenericCommands.Cut"), LOCTEXT("SplitTip", "Split the clip under the playhead into two clips"), [this]() { Timeline->SplitAtPlayhead(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				TimelineButton(Icons({ Bar(), Glyph("Icons.ChevronRight") }), LOCTEXT("TrimStartTip", "Trim Start: cut the part of the clip before the playhead"), [this]() { Timeline->TrimStartAtPlayhead(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				TimelineButton(Icons({ Glyph("Icons.ChevronLeft"), Bar() }), LOCTEXT("TrimEndTip", "Trim End: cut the part of the clip after the playhead"), [this]() { Timeline->TrimEndAtPlayhead(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 18, 3)
			[
				SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(5.f, 4.f))
				.ToolTipText(LOCTEXT("ZoomFitTip", "Zoom to Fit (Enter with no track selected)"))
				.OnClicked_Lambda([this]() { Timeline->ZoomToFit(); return FReply::Handled(); })
				[Icons({ Glyph("Icons.ChevronRight"), Glyph("Icons.ChevronLeft") })]
			]
			+ SWrapBox::Slot().Padding(0, 0, 6, 3)
			[
				SNew(SComboButton).HasDownArrow(true)
				.ToolTipText(LOCTEXT("AddCoreBoneTip", "Add a bone track: pelvis, spine, head or any other bone (bones already on the list are not offered)"))
				.ButtonContent()[SNew(STextBlock).Text(LOCTEXT("AddCoreBone", "Add Core Bone"))]
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget> { FMenuBuilder Menu(true, nullptr); Timeline->BuildCoreBoneMenu(Menu); return Menu.MakeWidget(); })
			]
			+ SWrapBox::Slot().Padding(0, 0, 18, 3)
			[
				SNew(SComboButton).HasDownArrow(true)
				.ToolTipText(LOCTEXT("AddLimbTip", "Add arm or leg tracks, FK (a control on each joint) or IK (effector + pole vector)"))
				.ButtonContent()[SNew(STextBlock).Text(LOCTEXT("AddLimb", "Add Limb"))]
				.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget> { FMenuBuilder Menu(true, nullptr); Timeline->BuildLimbMenu(Menu); return Menu.MakeWidget(); })
			]
			+ SWrapBox::Slot().VAlign(VAlign_Center).Padding(0, 0, 6, 3)
			[
				SNew(STextBlock).Text(LOCTEXT("CrossfadeCurve", "Crossfade curve:"))
			]
			+ SWrapBox::Slot().Padding(0, 0, 2, 3)
			[
				CurveButton(EAnimStitchBlendCurve::Linear, LOCTEXT("Linear", "Linear"))
			]
			+ SWrapBox::Slot()
			[
				CurveButton(EAnimStitchBlendCurve::Smoothstep, LOCTEXT("Smoothstep", "Ease In/Out"))
			]
		];

	// The toolbar reflows (wraps) to whatever width its panel has, and scrolls when even that is too short.
	ToolbarSection = SNew(SScrollBox) + SScrollBox::Slot()[ToolbarRows];

	TSharedRef<SWidget> TimelineBorder = SNew(SBorder).Padding(0)
		[
			SNew(SOverlay).Clipping(EWidgetClipping::ClipToBounds)
			+ SOverlay::Slot()
			[
				Timeline.ToSharedRef()
			]
			+ SOverlay::Slot()
			[
				// Value panels of the expanded tracks (created on demand by SyncKeyPanels); clicks fall through to the timeline outside them.
				SAssignNew(KeyPanelOverlay, SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Fill)
			.Padding(TAttribute<FMargin>::CreateLambda([this]() { return FMargin(0.f, Timeline.IsValid() ? Timeline->GetTrackAreaTop() : 0.f, 0.f, 0.f); }))
			[
				SAssignNew(TrackScrollBar, SScrollBar).Orientation(Orient_Vertical).Thickness(FVector2D(9.f, 9.f))
				.Visibility_Lambda([this]() { return Timeline.IsValid() && Timeline->GetTrackScrollMax() > 0.f ? EVisibility::Visible : EVisibility::Collapsed; })
				.OnUserScrolled(FOnUserScrolled::CreateLambda([this](float Offset) { if (Timeline.IsValid()) { Timeline->SetTrackScroll(Offset * Timeline->GetTrackContentHeight()); } }))
			]
		];

	TSharedRef<SWidget> TransportBar = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 2.f, 6.f, 2.f)
		[
			SNew(SAnimStitchTransportBar).Timeline(Timeline)
			.OnGoToStart([this]() { double S, E; Asset->GetPlaybackRange(S, E); Timeline->SetPlayhead(S); OnTimelineScrubbed(); })
			.OnPreviousKey([this]() { Timeline->JumpToAdjacentKey(-1); })
			.OnPlayPause([this]() { TogglePlayback(); })
			.OnNextKey([this]() { Timeline->JumpToAdjacentKey(1); })
			.OnGoToEnd([this]() { double S, E; Asset->GetPlaybackRange(S, E); Timeline->SetPlayhead(E); OnTimelineScrubbed(); })
			.OnStop([this]() { Stop(); })
			.OnToggleLoop([this]() { bLoop = !bLoop; })
			.OnSetPlayhead([this](double T) { Timeline->SetPlayhead(T); OnTimelineScrubbed(); })
			.IsPlaying([this]() { return bPlaying; })
			.IsLooping([this]() { return bLoop; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 2.f)
		[
			SNew(STextBlock).Text(this, &SAnimStitchPanel::GetStatusText).AutoWrapText(true)
		];

	// The timeline and its bottom bar (transport, time, scroll / zoom) are ONE dockable unit: they always move together.
	TimelineSection = SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.f)[TimelineBorder]
		+ SVerticalBox::Slot().AutoHeight()[TransportBar];

	InspectorSection = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 6.f, 6.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0)[RightTabButton(LOCTEXT("ContentBrowserTab", "Content Browser"), 0)]
			+ SHorizontalBox::Slot().AutoWidth()[RightTabButton(LOCTEXT("DetailsTab", "Details"), 1)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)[RightTabButton(LOCTEXT("BakedAssetsTab", "Baked Assets"), 2)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)[RightTabButton(LOCTEXT("ControlPropsTab", "Control Properties"), 3)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0)[RightTabButton(LOCTEXT("AttachedPropsTab", "Attached Props"), 4)]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SWidgetSwitcher).WidgetIndex_Lambda([this]() { return RightTab; })
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(BrowserBox, SBox)
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SAnimStitchBoneDetails).Preview(Preview).Timeline(Timeline)
				.LiveIK([this](int32 Track, FAnimStitchTrackKey& OutKey) { return Preview->GetIKLiveKey(Track, OutKey); })
			]
			+ SWidgetSwitcher::Slot()
			[
				MakeBakedAssetsTab()
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SAnimStitchControlProperties).Preview(Preview).Timeline(Timeline)
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SAnimStitchAttachedProps).Preview(Preview).Timeline(Timeline)
			]
		];
	RebuildSequenceBrowser();
	RebuildBakedList();
	FAnimStitchBaker::OnAssetBaked().AddSP(this, &SAnimStitchPanel::RebuildBakedList);

	// Restore the last preview mesh.
	const UAnimStitchSettings* Settings = GetDefault<UAnimStitchSettings>();
	if (Settings->LastPreviewMesh.IsValid())
	{
		if (USkeletalMesh* Last = Cast<USkeletalMesh>(Settings->LastPreviewMesh.TryLoad()))
		{
			MeshPath = Last->GetPathName();
			Preview->SetMesh(Last);
			Viewport->FocusOnMesh();
		}
	}

	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SAnimStitchPanel::UpdatePreview));
}

SAnimStitchPanel::~SAnimStitchPanel()
{
	FAnimStitchContentBrowserBadge::ClearClipUsage(); // the badges go away with the timeline
}

void SAnimStitchPanel::PublishClipUsage()
{
	TMap<FSoftObjectPath, int32> Usage;
	for (const FAnimStitchClip& Clip : Asset->Timeline.Clips)
	{
		if (Clip.Sequence) { Usage.FindOrAdd(FSoftObjectPath(Clip.Sequence)) += 1; }
	}
	FAnimStitchContentBrowserBadge::SetClipUsage(Usage);
}

void SAnimStitchPanel::SyncBrowserSelection(double Now)
{
	if (Now - LastBrowserPoll < 0.25) { return; }
	LastBrowserPoll = Now;
	TArray<FAssetData> Selected;
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowser.Get().GetSelectedAssets(Selected);
	if (BrowserSelectionDelegate.IsBound()) { Selected.Append(BrowserSelectionDelegate.Execute()); }
	FString Signature;
	for (const FAssetData& Data : Selected) { Signature += Data.GetSoftObjectPath().ToString() + TEXT(";"); }
	if (Signature == LastBrowserSelection) { return; }
	LastBrowserSelection = Signature;
	// Selecting a badged asset in the Content Browser selects its clip on the timeline.
	for (const FAssetData& Data : Selected)
	{
		if (Timeline->SelectClipForSequence(Data.GetSoftObjectPath())) { break; }
	}
}

void SAnimStitchPanel::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Asset);
}

EActiveTimerReturnType SAnimStitchPanel::UpdatePreview(double CurrentTime, float DeltaTime)
{
	if (Asset->GetEditCounter() != LastEditCounter)
	{
		SyncPreviewToTimeline();
	}
	Timeline->ApplyPendingZoomFit();
	SyncBrowserSelection(CurrentTime);
	SyncKeyPanels();
	if (Timeline->GetTrackSelectionSerial() != LastTrackSerial)
	{
		// Selecting (or adding) a bone track selects its bone and its control shape in the viewport.
		LastTrackSerial = Timeline->GetTrackSelectionSerial();
		const int32 SelectedTrack = Timeline->GetSelectedTrack();
		if (Asset->Timeline.Tracks.IsValidIndex(SelectedTrack))
		{
			const FAnimStitchTrack& Track = Asset->Timeline.Tracks[SelectedTrack];
			Preview->SelectBone(Preview->FindMeshBone(Track.Bone));
			FAnimStitchControlId Id;
			Id.Kind = Track.Kind == EAnimStitchTrackKind::IKLimb ? EAnimStitchControlKind::IKEffector : EAnimStitchControlKind::Bone;
			Id.A = SelectedTrack;
			Preview->SelectControl(Id);
		}
	}
	{
		// Selecting a control in the viewport selects its bone track in the timeline.
		const FAnimStitchControlId Control = Preview->GetSelectedControl();
		if ((int32)Control.Kind != LastControlKind || Control.A != LastControlTrack)
		{
			LastControlKind = (int32)Control.Kind;
			LastControlTrack = Control.A;
			if (Control.Kind == EAnimStitchControlKind::Bone || Control.Kind == EAnimStitchControlKind::IKEffector || Control.Kind == EAnimStitchControlKind::IKPole)
			{
				Timeline->SelectTrackFromViewport(Control.A);
			}
		}
	}
	if (TrackScrollBar.IsValid())
	{
		Timeline->SetTrackScroll(Timeline->GetTrackScroll()); // re-clamp when lanes were removed or the panel was resized
		const float Content = Timeline->GetTrackContentHeight();
		if (Content > 1.f) { TrackScrollBar->SetState(Timeline->GetTrackScroll() / Content, FMath::Min(1.f, Timeline->GetTrackViewHeight() / Content)); }
	}
	if (Asset->GetManualCounter() != LastManualCounter)
	{
		LastManualCounter = Asset->GetManualCounter();
		Preview->SetManualTracks(Asset->Timeline.Tracks);
		Preview->SetPreviewAttachments(Asset->Timeline.PreviewAttachments);
	}
	if (BoneItemsMesh.Get() != Preview->GetMesh())
	{
		RebuildBoneItems();
	}
	UpdatePlantLanes(CurrentTime);

	const USkeleton* MeshSkeleton = Preview->GetMesh() ? Preview->GetMesh()->GetSkeleton() : nullptr;
	if (BrowserSkeleton.Get() != MeshSkeleton)
	{
		RebuildSequenceBrowser();
	}

	double RangeStart, RangeEnd;
	Asset->GetPlaybackRange(RangeStart, RangeEnd);
	if (bPlaying && RangeEnd > RangeStart)
	{
		double T = Timeline->GetPlayhead() + DeltaTime;
		if (T >= RangeEnd)
		{
			if (bLoop)
			{
				T = RangeStart + FMath::Fmod(T - RangeStart, RangeEnd - RangeStart);
			}
			else
			{
				T = RangeEnd;
				bPlaying = false;
			}
		}
		Timeline->SetPlayhead(T);
	}

	if (Timeline->GetPlayhead() != LastPlayhead)
	{
		LastPlayhead = Timeline->GetPlayhead();
		Preview->SetPlayhead(LastPlayhead);
	}
	return EActiveTimerReturnType::Continue;
}

void SAnimStitchPanel::SyncPreviewToTimeline()
{
	LastEditCounter = Asset->GetEditCounter();
	PublishClipUsage();
	AutoSelectPreviewMesh();
	Preview->SetTimeline(Asset->Timeline);
	Timeline->SetPlayhead(Timeline->GetPlayhead()); // re-clamp to the new length
	LastPlayhead = Timeline->GetPlayhead();
	Preview->SetPlayhead(LastPlayhead);
}

void SAnimStitchPanel::UpdatePlantLanes(double Now)
{
	if (!Asset->Timeline.FootLock.bEnabled && !Asset->Timeline.bPredictFeet)
	{
		Preview->ClearCorrectedPoses();
		Preview->ClearLockInfo();
		if (PlantData.IsValid())
		{
			PlantData.Reset();
			Timeline->SetPlants(nullptr);
		}
		PlantCounter = MAX_uint32;
		return;
	}

	// Re-detect shortly after the last edit, so dragging stays smooth.
	if (Asset->GetEditCounter() != PlantCounter)
	{
		PlantCounter = Asset->GetEditCounter();
		PlantDirtyAt = Now;
		bPlantDirty = true;
	}
	if (!bPlantDirty) { return; }
	if (Timeline->IsDraggingFootLock())
	{
		// Dragging a foot-lock handle or region: refresh the lane about ten times a second so it follows the mouse.
		if (Now - LastPlantAnalysis < 0.1) { return; }
	}
	else if (Now - PlantDirtyAt < 0.25)
	{
		return;
	}
	LastPlantAnalysis = Now;
	bPlantDirty = false;

	constexpr double AnalysisFps = 30.0;
	TSharedRef<FAnimStitchFootLockResult> Result = MakeShared<FAnimStitchFootLockResult>();
	if (const FAnimStitchEvaluator* Evaluator = Preview->GetEvaluator())
	{
		const int32 NumFrames = FMath::RoundToInt(Evaluator->GetTimeline().GetTotalLength() * AnalysisFps) + 1;
		TArray<TArray<FTransform>> Poses;
		Poses.SetNum(NumFrames);
		for (int32 f = 0; f < NumFrames; ++f)
		{
			Evaluator->EvaluatePose(f / AnalysisFps, Poses[f]);
		}
		// The same passes, in the same order, as the bake (what you see is what bakes).
		bool bOk = true;
		if (Evaluator->GetTimeline().bPredictFeet)
		{
			const FAnimStitchPredictReport Report = FAnimStitchFootLock::PredictFeet(*Evaluator, Poses, AnalysisFps);
			if (!Report.bValid)
			{
				Result->Problem = Report.Problem;
				bOk = false;
			}
		}
		if (!Evaluator->GetTimeline().FootLock.bEnabled)
		{
			Preview->ClearLockInfo();
		}
		if (bOk && Evaluator->GetTimeline().FootLock.bEnabled)
		{
			*Result = FAnimStitchFootLock::Detect(*Evaluator, Poses, AnalysisFps);
			if (Result->bValid)
			{
				FAnimStitchPinOutput PinOutput;
				FAnimStitchFootLock::Pin(*Evaluator, Poses, *Result, &PinOutput);
				FAnimStitchFootLockResult LockCopy = *Result;
				Preview->SetLockInfo(MoveTemp(LockCopy), MoveTemp(PinOutput), AnalysisFps);
			}
			else
			{
				bOk = false;
			}
		}
		if (bOk)
		{
			Preview->SetCorrectedPoses(MoveTemp(Poses), AnalysisFps);
		}
		else
		{
			Preview->ClearCorrectedPoses();
		}
	}
	else
	{
		Result->Problem = LOCTEXT("PlantNoTimeline", "Add clips to see foot plants.");
	}
	PlantData = Result;
	Timeline->SetPlants(Asset->Timeline.FootLock.bEnabled ? TSharedPtr<const FAnimStitchFootLockResult>(Result) : nullptr);
}

void SAnimStitchPanel::SetAsset(UAnimStitchTimelineAsset* NewAsset)
{
	if (!NewAsset || NewAsset == Asset)
	{
		return;
	}
	bPlaying = false;
	Asset = NewAsset;
	Preview->SetAsset(NewAsset);
	LastManualCounter = MAX_uint32;
	Timeline->SetTimelineAsset(NewAsset);
	LastEditCounter = MAX_uint32; // forces a re-sync next tick
	LastSkeleton = nullptr;
	LastWarning = FText::GetEmpty();
	Timeline->RequestZoomToFit();
}

TSharedRef<SWidget> SAnimStitchPanel::MakeViewportToolbar()
{
	auto IconToggle = [this](const FName& Brush, const FText& Tip, TFunction<bool()> IsOn, TFunction<void()> OnClick)
	{
		return SNew(SCheckBox)
			.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
			.ToolTipText(Tip)
			.IsChecked_Lambda([IsOn]() { return IsOn() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([OnClick](ECheckBoxState) { OnClick(); })
			[
				SNew(SBox).WidthOverride(24.f).HeightOverride(24.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SImage).Image(FAppStyle::GetBrush(Brush)).ColorAndOpacity(FSlateColor::UseForeground())
				]
			];
	};

	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(6.f, 3.f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
		[
			IconToggle("EditorViewport.TranslateMode", LOCTEXT("GizmoMoveTip", "Move the selected control (W)"), [this]() { return Preview->GetGizmoMode() == EAnimStitchGizmoMode::Translate; }, [this]() { Preview->SetGizmoMode(EAnimStitchGizmoMode::Translate); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
		[
			IconToggle("EditorViewport.RotateMode", LOCTEXT("GizmoRotateTip", "Rotate the selected control (E)"), [this]() { return Preview->GetGizmoMode() == EAnimStitchGizmoMode::Rotate; }, [this]() { Preview->SetGizmoMode(EAnimStitchGizmoMode::Rotate); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
		[
			IconToggle("EditorViewport.ScaleMode", LOCTEXT("GizmoScaleTip", "Scale the selected control (R)"), [this]() { return Preview->GetGizmoMode() == EAnimStitchGizmoMode::Scale; }, [this]() { Preview->SetGizmoMode(EAnimStitchGizmoMode::Scale); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText_Lambda([this]()
			{
				return Preview->GetCoordSpace() == EAnimStitchCoordSpace::World
					? LOCTEXT("CoordWorldTip", "World coordinates. Click to switch to local (the control's own axes).")
					: LOCTEXT("CoordLocalTip", "Local coordinates. Click to switch to world.");
			})
			.ContentPadding(FMargin(2.f))
			.OnClicked_Lambda([this]()
			{
				Preview->SetCoordSpace(Preview->GetCoordSpace() == EAnimStitchCoordSpace::World ? EAnimStitchCoordSpace::Local : EAnimStitchCoordSpace::World);
				return FReply::Handled();
			})
			[
				SNew(SBox).WidthOverride(24.f).HeightOverride(24.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SImage).ColorAndOpacity(FSlateColor::UseForeground())
					.Image_Lambda([this]() { return FAppStyle::GetBrush(Preview->GetCoordSpace() == EAnimStitchCoordSpace::World ? "EditorViewport.RelativeCoordinateSystem_World" : "EditorViewport.RelativeCoordinateSystem_Local"); })
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
		[
			SNew(SComboButton)
			.HasDownArrow(true)
			.ToolTipText(LOCTEXT("ShowMenuTip", "Show flags: bones, controls, sockets, grid, preview attachments"))
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Visible")).ColorAndOpacity(FSlateColor::UseForeground())]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(LOCTEXT("ShowMenu", "Show"))]
			]
			.OnGetMenuContent(this, &SAnimStitchPanel::MakeShowMenu)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(260.f)
			[
				SAssignNew(BoneCombo, SSearchableComboBox)
				.OptionsSource(&BoneItems)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(*Item)); })
				.OnSelectionChanged(this, &SAnimStitchPanel::OnBoneComboChanged)
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						const int32 Bone = Preview->GetSelectedBone();
						return Bone != INDEX_NONE ? FText::FromName(Preview->GetMeshBoneName(Bone)) : LOCTEXT("PickBoneCombo", "Select a bone...");
					})
				]
			]
		]
	];
}

TSharedRef<SWidget> SAnimStitchPanel::MakeShowMenu()
{
	FMenuBuilder Menu(false, nullptr); // stays open while toggling, like the animation editor's Show menu
	Menu.BeginSection("SceneElements", LOCTEXT("SceneElements", "Scene Elements"));
	Menu.AddSubMenu(LOCTEXT("BonesMenu", "Bones"), LOCTEXT("BonesMenuTip", "How the skeleton of the preview character is drawn"), FNewMenuDelegate::CreateSP(this, &SAnimStitchPanel::FillBonesMenu));
	Menu.AddSubMenu(LOCTEXT("ControlsMenu", "Controls"), LOCTEXT("ControlsMenuTip", "Manual-correction control shapes: visibility and size"), FNewMenuDelegate::CreateSP(this, &SAnimStitchPanel::FillControlsMenu));
	Menu.AddSubMenu(LOCTEXT("AttachmentsMenu", "Preview Attachments"), LOCTEXT("AttachmentsMenuTip", "Props on the preview character (preview only, never baked)"), FNewMenuDelegate::CreateSP(this, &SAnimStitchPanel::FillAttachmentsMenu));
	Menu.EndSection();

	Menu.BeginSection("CommonShowFlags", LOCTEXT("CommonShowFlags", "Common Show Flags"));
	Menu.AddMenuEntry(LOCTEXT("GridFlag", "Grid"), LOCTEXT("GridFlagTip", "Show or hide the ground grid (Alt+Shift+G)"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { Preview->SetShowGrid(!Preview->GetShowGrid()); }), FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() { return Preview->GetShowGrid(); })),
		NAME_None, EUserInterfaceActionType::ToggleButton);
	Menu.EndSection();
	return Menu.MakeWidget();
}

void SAnimStitchPanel::FillControlsMenu(FMenuBuilder& Menu)
{
	Menu.BeginSection("ControlsShow", LOCTEXT("ControlsSection", "Controls"));
	Menu.AddMenuEntry(LOCTEXT("ShowControls", "Show Controls"), LOCTEXT("ShowControlsTip", "Show the control shapes and the manipulator for manual corrections"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { Preview->SetShowControls(!Preview->GetShowControls()); }), FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]() { return Preview->GetShowControls(); })),
		NAME_None, EUserInterfaceActionType::ToggleButton);
	Menu.AddWidget(
		SNew(SBox).WidthOverride(120.f)
		[
			SNew(SSpinBox<float>).MinValue(0.2f).MaxValue(8.f).MinSliderValue(0.2f).MaxSliderValue(4.f).Delta(0.1f)
			.ToolTipText(LOCTEXT("ControlSizeTip", "Size of the control shapes"))
			.Value_Lambda([this]() { return Preview->GetControlDrawSize(); })
			.OnValueChanged_Lambda([this](float V) { Preview->SetControlDrawSize(V); })
			.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { Preview->SetControlDrawSize(V); })
		],
		LOCTEXT("ControlSize", "Control Size"));
	Menu.AddWidget(
		SNew(SBox).WidthOverride(120.f)
		[
			SNew(SSpinBox<float>).MinValue(0.3f).MaxValue(4.f).MinSliderValue(0.3f).MaxSliderValue(3.f).Delta(0.1f)
			.ToolTipText(LOCTEXT("GizmoSizeTip", "Size of the move / rotate / scale manipulator"))
			.Value_Lambda([this]() { return Preview->GetGizmoDrawSize(); })
			.OnValueChanged_Lambda([this](float V) { Preview->SetGizmoDrawSize(V); })
			.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { Preview->SetGizmoDrawSize(V); })
		],
		LOCTEXT("GizmoSize", "Gizmo Size"));
	Menu.AddWidget(
		SNew(SBox).WidthOverride(120.f)
		[
			SNew(SSpinBox<float>).MinValue(0.02f).MaxValue(1.f).MinSliderValue(0.02f).MaxSliderValue(1.f).Delta(0.02f)
			.ToolTipText(LOCTEXT("ControlOpacityTip", "Opacity of the control cubes (low = see the mesh through them)"))
			.Value_Lambda([this]() { return Preview->GetControlOpacity(); })
			.OnValueChanged_Lambda([this](float V) { Preview->SetControlOpacity(V); })
			.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { Preview->SetControlOpacity(V); })
		],
		LOCTEXT("ControlOpacity", "Control Opacity"));
	Menu.EndSection();
}

void SAnimStitchPanel::FillAttachmentsMenu(FMenuBuilder& Menu)
{
	Menu.AddMenuEntry(LOCTEXT("AttachAsset", "Attach Asset..."), LOCTEXT("AttachAssetTip", "Attach a static or skeletal mesh to a socket or bone of the preview character"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { SAnimStitchAttachDialog::Show(Asset, Preview->GetMesh()); }),
			FCanExecuteAction::CreateLambda([this]() { return Preview->GetMesh() != nullptr; })));
	if (Asset->Timeline.PreviewAttachments.Num() > 0)
	{
		Menu.BeginSection("CurrentAttachments", LOCTEXT("CurrentAttachments", "Attached"));
		for (int32 i = 0; i < Asset->Timeline.PreviewAttachments.Num(); ++i)
		{
			const FAnimStitchPreviewAttachment& Attachment = Asset->Timeline.PreviewAttachments[i];
			Menu.AddMenuEntry(FText::Format(LOCTEXT("RemoveAttachment", "Remove {0} @ {1}"), FText::FromString(Attachment.Asset.GetAssetName()), FText::FromName(Attachment.Socket)), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, i]()
				{
					const FScopedTransaction Tx(LOCTEXT("RemoveAttachTx", "Remove Preview Attachment"));
					Asset->RemovePreviewAttachment(i);
				})));
		}
		Menu.EndSection();
	}
}

void SAnimStitchPanel::FillBonesMenu(FMenuBuilder& Menu)
{
	auto Toggle = [this, &Menu](const FText& Label, const FText& Tip, TFunction<bool()> Get, TFunction<void(bool)> Set)
	{
		Menu.AddMenuEntry(Label, Tip, FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Get, Set]() { Set(!Get()); }), FCanExecuteAction(), FIsActionChecked::CreateLambda([Get]() { return Get(); })),
			NAME_None, EUserInterfaceActionType::ToggleButton);
	};

	Menu.BeginSection("BonesShow", LOCTEXT("BonesShow", "Show"));
	Toggle(LOCTEXT("ShowSockets", "Sockets"), LOCTEXT("ShowSocketsTip", "Draw the preview mesh's sockets"), [this]() { return Preview->GetShowSockets(); }, [this](bool b) { Preview->SetShowSockets(b); });
	Toggle(LOCTEXT("ShowBoneNames", "Bone Names"), LOCTEXT("ShowBoneNamesTip", "Label the drawn bones with their names"), [this]() { return Preview->GetShowBoneNames(); }, [this](bool b) { Preview->SetShowBoneNames(b); });
	Toggle(LOCTEXT("MultiColorBones", "Multi-Color Bones"), LOCTEXT("MultiColorBonesTip", "Colour bones by their depth in the hierarchy"), [this]() { return Preview->GetMultiColorBones(); }, [this](bool b) { Preview->SetMultiColorBones(b); });
	Menu.EndSection();

	Menu.BeginSection("BoneDrawing", LOCTEXT("BoneDrawing", "Bone Drawing"));
	Menu.AddWidget(
		SNew(SBox).WidthOverride(120.f)
		[
			SNew(SSpinBox<float>).MinValue(0.1f).MaxValue(10.f).MinSliderValue(0.1f).MaxSliderValue(5.f).Delta(0.1f)
			.ToolTipText(LOCTEXT("BoneDrawSizeTip", "Size of the drawn joints, links and axes"))
			.Value_Lambda([this]() { return Preview->GetBoneDrawSize(); })
			.OnValueChanged_Lambda([this](float V) { Preview->SetBoneDrawSize(V); })
			.OnValueCommitted_Lambda([this](float V, ETextCommit::Type) { Preview->SetBoneDrawSize(V); })
		],
		LOCTEXT("BoneDrawSize", "Bone Draw Size"));
	auto Mode = [this, &Menu](const FText& Label, const FText& Tip, EAnimStitchBoneDrawMode Value)
	{
		Menu.AddMenuEntry(Label, Tip, FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Value]() { Preview->SetBoneDrawMode(Value); }), FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, Value]() { return Preview->GetBoneDrawMode() == Value; })),
			NAME_None, EUserInterfaceActionType::RadioButton);
	};
	Mode(LOCTEXT("ModeAll", "All Hierarchy"), LOCTEXT("ModeAllTip", "Draw every bone. Click a joint to select it."), EAnimStitchBoneDrawMode::AllHierarchy);
	Mode(LOCTEXT("ModeSelected", "Selected Only"), LOCTEXT("ModeSelectedTip", "Draw only the selected bone"), EAnimStitchBoneDrawMode::SelectedOnly);
	Mode(LOCTEXT("ModeParents", "Selected and Parents"), LOCTEXT("ModeParentsTip", "The selected bone and everything above it"), EAnimStitchBoneDrawMode::SelectedAndParents);
	Mode(LOCTEXT("ModeChildren", "Selected and Children"), LOCTEXT("ModeChildrenTip", "The selected bone and everything below it"), EAnimStitchBoneDrawMode::SelectedAndChildren);
	Mode(LOCTEXT("ModeBoth", "Selected and Parents and Children"), LOCTEXT("ModeBothTip", "The selected bone, its ancestors and its descendants"), EAnimStitchBoneDrawMode::SelectedAndParentsAndChildren);
	Mode(LOCTEXT("ModeNone", "None"), LOCTEXT("ModeNoneTip", "Don't draw the skeleton"), EAnimStitchBoneDrawMode::None);
	Menu.EndSection();
}

void SAnimStitchPanel::RebuildBoneItems()
{
	BoneItemsMesh = Preview->GetMesh();
	BoneItems.Reset();
	for (int32 i = 0; i < Preview->GetNumMeshBones(); ++i)
	{
		BoneItems.Add(MakeShared<FString>(Preview->GetMeshBoneName(i).ToString()));
	}
	if (BoneCombo.IsValid())
	{
		BoneCombo->RefreshOptions();
		OnPreviewBoneSelected();
	}
}

void SAnimStitchPanel::OnBoneComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo == ESelectInfo::Direct || !Item.IsValid())
	{
		return; // the change came from the viewport (or a refresh), not from the user picking in the list
	}
	Preview->SelectBone(Preview->FindMeshBone(FName(**Item)));
	RightTab = 1; // picking a bone in the dropdown shows its Details
}

void SAnimStitchPanel::OnPreviewBoneSelected()
{
	if (!BoneCombo.IsValid())
	{
		return;
	}
	const int32 Bone = Preview->GetSelectedBone();
	if (Bone == INDEX_NONE)
	{
		BoneCombo->ClearSelection();
		return;
	}
	const FString Name = Preview->GetMeshBoneName(Bone).ToString();
	for (const TSharedPtr<FString>& Item : BoneItems)
	{
		if (*Item == Name)
		{
			BoneCombo->SetSelectedItem(Item);
			break;
		}
	}
}

void SAnimStitchPanel::OnTimelineScrubbed()
{
	// Scrubbing stops playback and poses the character right away, instead of waiting for the next UI tick.
	Pause();
	LastPlayhead = Timeline->GetPlayhead();
	Preview->SetPlayhead(LastPlayhead);
}

void SAnimStitchPanel::OpenFromDisk()
{
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FOpenAssetDialogConfig Config;
	Config.DialogTitleOverride = LOCTEXT("OpenTimelineTitle", "Open AnimStitch Timeline");
	Config.DefaultPath = Asset->GetOutermost() == GetTransientPackage() ? FString(TEXT("/Game")) : FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	Config.AssetClassNames.Add(UAnimStitchTimelineAsset::StaticClass()->GetClassPathName());
	Config.bAllowMultipleSelection = false;
	const TWeakPtr<SAnimStitchPanel> WeakPanel = SharedThis(this);
	ContentBrowser.Get().CreateOpenAssetDialog(Config,
		FOnAssetsChosenForOpen::CreateLambda([WeakPanel](const TArray<FAssetData>& Chosen)
		{
			if (const TSharedPtr<SAnimStitchPanel> Panel = WeakPanel.Pin())
			{
				if (Chosen.Num() > 0 && Panel->ConfirmDiscardChanges())
				{
					Panel->SetAsset(Cast<UAnimStitchTimelineAsset>(Chosen[0].GetAsset()));
				}
			}
		}),
		FOnAssetDialogCancelled());
}

TSharedPtr<SWidget> SAnimStitchPanel::OnGetAssetContextMenu(const TArray<FAssetData>& Selected)
{
	const TArray<FAssetData> Assets = Selected;
	FMenuBuilder Menu(true, nullptr);
	Menu.AddMenuEntry(LOCTEXT("CbAddToTimeline", "Add to Timeline"), LOCTEXT("CbAddToTimelineTip", "Append the selected animation sequences to the timeline"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Assets]()
		{
			TArray<UAnimSequence*> Sequences;
			for (const FAssetData& Data : Assets)
			{
				if (UAnimSequence* Seq = Cast<UAnimSequence>(Data.GetAsset())) { Sequences.Add(Seq); }
			}
			Timeline->AddSequencesToEnd(Sequences);
		})));
	Menu.AddMenuEntry(LOCTEXT("CbOpenEditor", "Open in Animation Editor"), LOCTEXT("CbOpenEditorTip", "Open the animation sequence in the animation editor"),
		FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([Assets]()
		{
			if (GEditor)
			{
				if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
				{
					for (const FAssetData& Data : Assets) { Subsystem->OpenEditorForAsset(Data.GetAsset()); }
				}
			}
		})));
	Menu.AddMenuEntry(LOCTEXT("CbBrowse", "Browse to Asset"), LOCTEXT("CbBrowseTip", "Show the asset in the main Content Browser"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
		FUIAction(FExecuteAction::CreateLambda([Assets]()
		{
			if (GEditor)
			{
				TArray<FAssetData> ToSync = Assets;
				GEditor->SyncBrowserToObjects(ToSync);
			}
		})));
	return Menu.MakeWidget();
}

bool SAnimStitchPanel::bSkipLayoutPersist = false;

namespace
{
	const FName ToolbarTabId(TEXT("AnimStitch.Toolbar"));
	const FName ViewportTabId(TEXT("AnimStitch.Viewport"));
	const FName TimelineTabId(TEXT("AnimStitch.Timeline"));
	const FName InspectorTabId(TEXT("AnimStitch.Inspector"));
}

TSharedRef<FTabManager::FLayout> SAnimStitchPanel::MakeDefaultLayout() const
{
	return FTabManager::NewLayout("AnimStitchLayout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split(FTabManager::NewStack()->SetSizeCoefficient(0.14f)->AddTab(ToolbarTabId, ETabState::OpenedTab))
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.86f)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)->SetSizeCoefficient(0.74f)
					->Split(FTabManager::NewStack()->SetSizeCoefficient(0.52f)->AddTab(ViewportTabId, ETabState::OpenedTab))
					->Split(FTabManager::NewStack()->SetSizeCoefficient(0.48f)->AddTab(TimelineTabId, ETabState::OpenedTab))
				)
				->Split(FTabManager::NewStack()->SetSizeCoefficient(0.26f)->AddTab(InspectorTabId, ETabState::OpenedTab))
			)
		);
}

void SAnimStitchPanel::InitializeDocking(const TSharedRef<SDockTab>& Tab)
{
	TabManager = FGlobalTabmanager::Get()->NewTabManager(Tab);
	TabManager->SetCanDoDragOperation(true);

	auto Register = [this](FName Id, const FText& Name, const TSharedPtr<SWidget>& Section, bool bCanClose)
	{
		TabManager->RegisterTabSpawner(Id, FOnSpawnTab::CreateLambda([Section, Name, bCanClose](const FSpawnTabArgs&) -> TSharedRef<SDockTab>
		{
			TSharedRef<SDockTab> SectionTab = SNew(SDockTab).TabRole(ETabRole::PanelTab).Label(Name)[Section.ToSharedRef()];
			if (!bCanClose) { SectionTab->SetCanCloseTab(SDockTab::FCanCloseTab::CreateLambda([]() { return false; })); }
			return SectionTab;
		})).SetDisplayName(Name);
	};
	// The toolbar (it holds the Layout menu) and the viewport cannot be closed; the other sections can, and come back from Layout.
	Register(ToolbarTabId, LOCTEXT("ToolbarTab", "Toolbar"), ToolbarSection, false);
	Register(ViewportTabId, LOCTEXT("ViewportTab", "Viewport"), ViewportSection, false);
	Register(TimelineTabId, LOCTEXT("TimelineTab", "Timeline"), TimelineSection, true);
	Register(InspectorTabId, LOCTEXT("InspectorTab", "Inspector"), InspectorSection, true);

	// Layout preferences save themselves whenever the layout changes.
	TabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateLambda([](const TSharedRef<FTabManager::FLayout>& InLayout)
	{
		if (!bSkipLayoutPersist) { FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, InLayout); }
	}));

	TSharedRef<FTabManager::FLayout> Layout = FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, MakeDefaultLayout());
	TSharedPtr<SWidget> Restored = TabManager->RestoreFrom(Layout, TSharedPtr<SWindow>(), false);
	if (!Restored.IsValid())
	{
		Restored = TabManager->RestoreFrom(MakeDefaultLayout(), TSharedPtr<SWindow>(), false);
	}
	ChildSlot
	[
		Restored.IsValid() ? Restored.ToSharedRef() : SNullWidget::NullWidget
	];
}

TSharedRef<SWidget> SAnimStitchPanel::MakeLayoutMenu()
{
	FMenuBuilder Menu(true, nullptr);
	Menu.AddMenuEntry(LOCTEXT("ResetLayout", "Reset to Default Layout"), LOCTEXT("ResetLayoutTip", "Put every section back where it started (asks first)"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchPanel::ResetLayout)));
	Menu.AddMenuEntry(LOCTEXT("SaveLayoutAs", "Save Layout As..."), LOCTEXT("SaveLayoutAsTip", "Save the current panel arrangement under a name of your own"), FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SAnimStitchPanel::SaveLayoutAs)));
	const TMap<FString, FString>& Saved = GetDefault<UAnimStitchSettings>()->SavedLayouts;
	TArray<FString> Names;
	Saved.GetKeys(Names);
	Names.Sort();
	Menu.AddSubMenu(LOCTEXT("LoadLayoutMenu", "Load Layout"), LOCTEXT("LoadLayoutMenuTip", "Switch to one of your saved layouts"),
		FNewMenuDelegate::CreateLambda([this, Names](FMenuBuilder& Sub)
		{
			if (Names.Num() == 0) { Sub.AddMenuEntry(LOCTEXT("NoSavedLayouts", "No saved layouts yet"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; }))); }
			for (const FString& Name : Names) { Sub.AddMenuEntry(FText::FromString(Name), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Name]() { LoadSavedLayout(Name); }))); }
		}), false, FSlateIcon(), false);
	Menu.AddSubMenu(LOCTEXT("DeleteLayoutMenu", "Delete Saved Layout"), LOCTEXT("DeleteLayoutMenuTip", "Remove one of your saved layouts"),
		FNewMenuDelegate::CreateLambda([this, Names](FMenuBuilder& Sub)
		{
			if (Names.Num() == 0) { Sub.AddMenuEntry(LOCTEXT("NoSavedLayouts2", "No saved layouts yet"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; }))); }
			for (const FString& Name : Names) { Sub.AddMenuEntry(FText::FromString(Name), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Name]() { DeleteSavedLayout(Name); }))); }
		}), false, FSlateIcon(), false);
	Menu.AddSeparator();
	// Sections that were closed can be brought back.
	auto ShowEntry = [this, &Menu](const FText& Label, FName Id)
	{
		Menu.AddMenuEntry(Label, FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Id]() { if (TabManager.IsValid()) { TabManager->TryInvokeTab(FTabId(Id)); } })));
	};
	ShowEntry(LOCTEXT("ShowTimeline", "Show Timeline"), TimelineTabId);
	ShowEntry(LOCTEXT("ShowInspector", "Show Inspector"), InspectorTabId);
	return Menu.MakeWidget();
}

void SAnimStitchPanel::SaveLayoutAs()
{
	if (!TabManager.IsValid()) { return; }
	TSharedRef<FString> Name = MakeShared<FString>();
	TSharedRef<SWindow> Window = SNew(SWindow).Title(LOCTEXT("SaveLayoutTitle", "Save Layout")).ClientSize(FVector2D(360.0, 120.0)).SupportsMinimize(false).SupportsMaximize(false);
	TWeakPtr<SWindow> WeakWindow = Window;
	auto Commit = [this, Name, WeakWindow]()
	{
		const FString Trimmed = Name->TrimStartAndEnd();
		if (!Trimmed.IsEmpty() && TabManager.IsValid())
		{
			// Stored in the AnimStitch settings; the current layout keeps auto-saving on its own.
			UAnimStitchSettings* Settings = GetMutableDefault<UAnimStitchSettings>();
			Settings->SavedLayouts.Add(Trimmed, TabManager->PersistLayout()->ToString());
			Settings->SaveConfig();
		}
		if (TSharedPtr<SWindow> W = WeakWindow.Pin()) { W->RequestDestroyWindow(); }
	};
	Window->SetContent(
		SNew(SBox).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[SNew(STextBlock).Text(LOCTEXT("SaveLayoutPrompt", "Name for this layout:"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SEditableTextBox)
				.OnTextChanged_Lambda([Name](const FText& Text) { *Name = Text.ToString(); })
				.OnTextCommitted_Lambda([Name, Commit](const FText& Text, ETextCommit::Type Type) { *Name = Text.ToString(); if (Type == ETextCommit::OnEnter) { Commit(); } })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SButton).Text(LOCTEXT("SaveLayoutOk", "Save"))
				.OnClicked_Lambda([Commit]() { Commit(); return FReply::Handled(); })
			]
		]);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SAnimStitchPanel::LoadSavedLayout(const FString& Name)
{
	const FString* Text = GetDefault<UAnimStitchSettings>()->SavedLayouts.Find(Name);
	if (!Text) { return; }
	const TSharedPtr<FTabManager::FLayout> Layout = FTabManager::FLayout::NewFromString(*Text);
	if (!Layout.IsValid()) { return; }
	// The chosen layout becomes the current one (and keeps auto-saving from there): write it as the current layout, then re-open the tab.
	bSkipLayoutPersist = true;
	FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, Layout.ToSharedRef());
	FAnimStitchEditorModule::RequestLayoutReset(Asset);
}

void SAnimStitchPanel::DeleteSavedLayout(const FString& Name)
{
	UAnimStitchSettings* Settings = GetMutableDefault<UAnimStitchSettings>();
	Settings->SavedLayouts.Remove(Name);
	Settings->SaveConfig();
}

void SAnimStitchPanel::ResetLayout()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("ResetLayoutConfirm", "Reset all sections (toolbar, viewport, timeline, inspector) to the default layout?\n\nYour timeline stays as it is.")) != EAppReturnType::Yes)
	{
		return;
	}
	bSkipLayoutPersist = true; // the layout being closed must not overwrite the default saved below
	FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, MakeDefaultLayout());
	FAnimStitchEditorModule::RequestLayoutReset(Asset);
}

void SAnimStitchPanel::SyncKeyPanels()
{
	if (!KeyPanelOverlay.IsValid()) { return; }
	while (KeyPanelCount < Asset->Timeline.Tracks.Num())
	{
		const int32 TrackIndex = KeyPanelCount++;
		KeyPanelOverlay->AddSlot().VAlign(VAlign_Top).HAlign(HAlign_Left)
		.Padding(TAttribute<FMargin>::CreateLambda([this, TrackIndex]() { return FMargin(0.f, Timeline->GetTrackExpanderTop(TrackIndex), 0.f, 0.f); }))
		[
			SNew(SBox).WidthOverride_Lambda([this]() -> FOptionalSize { return FOptionalSize(Timeline->GetLabelWidth() - 2.f); })
			.Visibility_Lambda([this, TrackIndex]()
			{
				if (!Timeline->IsTrackExpanded(TrackIndex)) { return EVisibility::Collapsed; }
				const float Top = Timeline->GetTrackExpanderTop(TrackIndex);
				return (Top >= Timeline->GetTrackAreaTop() - 1.f && Top < Timeline->GetTrackAreaTop() + Timeline->GetTrackViewHeight() - 24.f) ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SAnimStitchKeyPanel).Timeline(Timeline).TrackIndex(TrackIndex)
				.LiveIK([this](int32 Track, FAnimStitchTrackKey& OutKey) { return Preview->GetIKLiveKey(Track, OutKey); })
			]
		];
	}
}

void SAnimStitchPanel::RebuildBakedList()
{
	BakedItems.Reset();
	if (!ThumbnailPool.IsValid()) { ThumbnailPool = MakeShared<FAssetThumbnailPool>(64); }
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	const TArray<FSoftObjectPath>& Baked = GetDefault<UAnimStitchSettings>()->BakedAssets;
	for (int32 i = Baked.Num() - 1; i >= 0; --i) // newest first
	{
		const FAssetData Data = Registry.GetAssetByObjectPath(Baked[i]);
		if (!Data.IsValid()) { continue; }
		TSharedRef<FBakedItem> Item = MakeShared<FBakedItem>();
		Item->Path = Baked[i];
		Item->Thumbnail = MakeShared<FAssetThumbnail>(Data, 96, 96, ThumbnailPool);
		BakedItems.Add(Item);
	}
	if (BakedList.IsValid()) { BakedList->RequestListRefresh(); }
}

TSharedRef<SWidget> SAnimStitchPanel::MakeBakedAssetsTab()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f)
		[
			SNew(STextBlock).Text(LOCTEXT("NoBakedAssets", "Nothing baked yet. Animations you bake with AnimStitch are listed here: double-click to open, right-click to show in the Content Browser."))
			.AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Visibility_Lambda([this]() { return BakedItems.Num() == 0 ? EVisibility::Visible : EVisibility::Collapsed; })
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			// Content Browser style: a tile with a thumbnail and the name.
			SAssignNew(BakedList, STileView<TSharedPtr<FBakedItem>>)
			.ListItemsSource(&BakedItems)
			.SelectionMode(ESelectionMode::Single)
			.ItemWidth(112.f).ItemHeight(138.f)
			.ItemAlignment(EListItemAlignment::EvenlyDistributed)
			.OnGenerateTile_Lambda([](TSharedPtr<FBakedItem> Item, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
			{
				return SNew(STableRow<TSharedPtr<FBakedItem>>, Owner)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4.f, 4.f, 4.f, 2.f)
					[
						SNew(SBox).WidthOverride(96.f).HeightOverride(96.f)[Item->Thumbnail->MakeThumbnailWidget()]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(2.f, 0.f, 2.f, 4.f)
					[
						SNew(SBox).WidthOverride(104.f)
						[
							SNew(STextBlock).Text(FText::FromString(Item->Path.GetAssetName())).Justification(ETextJustify::Center).AutoWrapText(true)
							.ToolTipText(FText::FromString(Item->Path.ToString()))
						]
					]
				];
			})
			.OnMouseButtonDoubleClick_Lambda([](TSharedPtr<FBakedItem> Item)
			{
				// Opens in the Animation editor.
				if (UObject* Object = Item.IsValid() ? Item->Path.TryLoad() : nullptr)
				{
					if (GEditor) { GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object); }
				}
			})
			.OnContextMenuOpening_Lambda([this]() -> TSharedPtr<SWidget>
			{
				const TArray<TSharedPtr<FBakedItem>> Selected = BakedList->GetSelectedItems();
				if (Selected.Num() == 0) { return nullptr; }
				const FSoftObjectPath Path = Selected[0]->Path;
				FMenuBuilder Menu(true, nullptr);
				Menu.AddMenuEntry(LOCTEXT("ShowInContentBrowser", "Show in Content Browser"), FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([Path]()
					{
						if (UObject* Object = Path.TryLoad())
						{
							if (GEditor) { GEditor->SyncBrowserToObjects(TArray<UObject*>{ Object }); }
						}
					})));
				return Menu.MakeWidget();
			})
		];
}
FText SAnimStitchPanel::GetTabTitle() const
{
	if (Asset->GetOutermost() == GetTransientPackage())
	{
		return LOCTEXT("TabTitleUntitled", "AnimStitch");
	}
	return FText::Format(LOCTEXT("TabTitleAsset", "{0}{1}"), FText::FromString(Asset->GetName()), Asset->GetOutermost()->IsDirty() ? FText::FromString(TEXT("*")) : FText::GetEmpty());
}

FText SAnimStitchPanel::GetAssetLabel() const
{
	if (Asset->GetOutermost() == GetTransientPackage())
	{
		return LOCTEXT("Untitled", "Untitled timeline (not saved)");
	}
	return FText::Format(LOCTEXT("AssetLabel", "{0}{1}"), FText::FromString(Asset->GetName()), Asset->GetOutermost()->IsDirty() ? FText::FromString(TEXT("*")) : FText::GetEmpty());
}

bool SAnimStitchPanel::IsDirtyState() const
{
	const UPackage* Package = Asset->GetOutermost();
	return Asset->HasUnsavedChanges() || (Package != GetTransientPackage() && Package->IsDirty());
}

bool SAnimStitchPanel::ConfirmDiscardChanges()
{
	if (!IsDirtyState()) { return true; }
	const bool bUntitled = Asset->GetOutermost() == GetTransientPackage();
	const EAppReturnType::Type Answer = FMessageDialog::Open(EAppMsgType::YesNoCancel,
		FText::Format(LOCTEXT("SaveFirstPrompt", "Save changes to {0} before continuing?\n\nYes saves, No discards the changes, Cancel keeps working."), bUntitled ? LOCTEXT("UntitledName", "the untitled timeline") : FText::FromString(Asset->GetName())));
	if (Answer == EAppReturnType::Cancel) { return false; }
	if (Answer == EAppReturnType::Yes)
	{
		Save();
		if (IsDirtyState()) { return false; } // the save was cancelled or did not finish
	}
	return true;
}

void SAnimStitchPanel::NewTimeline()
{
	if (!ConfirmDiscardChanges()) { return; }
	SetAsset(NewObject<UAnimStitchTimelineAsset>(GetTransientPackage(), NAME_None, RF_Transactional));
}

void SAnimStitchPanel::Save()
{
	if (Asset->GetOutermost() == GetTransientPackage())
	{
		SaveAs();
		return;
	}
	FEditorFileUtils::PromptForCheckoutAndSave({ Asset->GetOutermost() }, false, false);
	if (!Asset->GetOutermost()->IsDirty()) { Asset->ClearUnsavedChanges(); }
}

void SAnimStitchPanel::SaveAs()
{
	FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UAnimStitchTimelineFactory* Factory = NewObject<UAnimStitchTimelineFactory>();
	const FString Folder = Asset->GetOutermost() == GetTransientPackage() ? FString(TEXT("/Game")) : FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	UAnimStitchTimelineAsset* Created = Cast<UAnimStitchTimelineAsset>(AssetTools.Get().CreateAssetWithDialog(TEXT("NewAnimStitchTimeline"), Folder, UAnimStitchTimelineAsset::StaticClass(), Factory));
	if (!Created)
	{
		return; // cancelled
	}
	Created->Timeline = Asset->Timeline;
	Created->PlaybackStart = Asset->PlaybackStart;
	Created->PlaybackEnd = Asset->PlaybackEnd;
	Created->MarkPackageDirty();
	SetAsset(Created);
	FEditorFileUtils::PromptForCheckoutAndSave({ Created->GetOutermost() }, false, false);
}

void SAnimStitchPanel::OpenBake()
{
	const UAnimStitchSettings* Settings = GetDefault<UAnimStitchSettings>();
	const bool bUntitled = Asset->GetOutermost() == GetTransientPackage();
	FString Folder = Settings->DefaultOutputPath.Path;
	if (Folder.IsEmpty())
	{
		Folder = bUntitled ? FString(TEXT("/Game")) : FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	}
	FString BaseName = bUntitled ? FString(TEXT("Merged")) : Asset->GetName();
	BaseName.RemoveFromStart(TEXT("TL_"));
	SAnimStitchBakeDialog::Show(Asset, Folder, TEXT("AS_") + BaseName, Settings->DefaultFrameRate);
}

void SAnimStitchPanel::OpenBatch()
{
	FString Folder = GetDefault<UAnimStitchSettings>()->DefaultOutputPath.Path;
	if (Folder.IsEmpty())
	{
		Folder = Asset->GetOutermost() == GetTransientPackage() ? FString(TEXT("/Game")) : FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	}
	SAnimStitchBatchDialog::Show(Asset,
		[this]() { return BrowserSelectionDelegate.IsBound() ? BrowserSelectionDelegate.Execute() : TArray<FAssetData>(); },
		Folder, GetDefault<UAnimStitchSettings>()->DefaultFrameRate);
}

void SAnimStitchPanel::RebuildSequenceBrowser()
{
	USkeleton* Skeleton = Preview->GetMesh() ? Preview->GetMesh()->GetSkeleton() : nullptr;
	BrowserSkeleton = Skeleton;
	if (!Skeleton)
	{
		BrowserBox->SetContent(
			SNew(SBox).Padding(12.f).VAlign(VAlign_Top)
			[
				SNew(STextBlock).Text(LOCTEXT("BrowserNoMesh", "Choose a preview skeletal mesh to list the animation sequences that use its skeleton."))
				.AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]);
		return;
	}

	FAssetPickerConfig Config;
	Config.Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	Config.Filter.bRecursiveClasses = true;
	Config.Filter.TagsAndValues.Add(FName("Skeleton"), FAssetData(Skeleton).GetExportTextName());
	Config.InitialAssetViewType = EAssetViewType::Tile; // thumbnails; hovering shows the standard asset tooltip with a live preview
	Config.bCanShowRealTimeThumbnails = true;
	Config.OnIsAssetValidForCustomToolTip = FOnIsAssetValidForCustomToolTip::CreateLambda([](FAssetData& Data)
	{
		return Data.GetClass() && Data.GetClass()->IsChildOf(UAnimSequence::StaticClass());
	});
	Config.OnGetCustomAssetToolTip = FOnGetCustomAssetToolTip::CreateLambda([this](FAssetData& Data)
	{
		return SNew(SToolTip)
		[
			SNew(SAnimStitchHoverPreview)
			.Sequence(Cast<UAnimSequence>(Data.GetAsset()))
			.Mesh(Preview->GetMesh())
			.PackageName(Data.PackageName.ToString())
		];
	});
	Config.OnGetAssetContextMenu =FOnGetAssetContextMenu::CreateSP(this, &SAnimStitchPanel::OnGetAssetContextMenu);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([](const FAssetData& Data)
	{
		if (GEditor)
		{
			if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				Subsystem->OpenEditorForAsset(Data.GetAsset());
			}
		}
	});
	Config.bAllowNullSelection = false;
	Config.bFocusSearchBoxWhenOpened = false;
	Config.bCanShowFolders = false;
	Config.bAllowDragging = true; // drag a sequence onto the timeline
	Config.SelectionMode = ESelectionMode::Multi;
	Config.GetCurrentSelectionDelegates.Add(&BrowserSelectionDelegate);
	Config.AssetShowWarningText =LOCTEXT("BrowserEmpty", "No animation sequences use this skeleton.");

	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	BrowserBox->SetContent(ContentBrowser.Get().CreateAssetPicker(Config));
}

void SAnimStitchPanel::AutoSelectPreviewMesh()
{
	// When the timeline gets a (new) skeleton and the current mesh doesn't fit it, pick a fitting mesh.
	USkeleton* Skeleton = Asset->GetSkeleton();
	if (!Skeleton)
	{
		LastSkeleton = nullptr;
		return;
	}
	if (LastSkeleton.Get() == Skeleton)
	{
		return;
	}
	LastSkeleton = Skeleton;
	if (Preview->GetMesh() && Preview->GetMesh()->GetSkeleton() == Skeleton)
	{
		return;
	}
	if (USkeletalMesh* Candidate = Skeleton->GetPreviewMesh(true))
	{
		MeshPath = Candidate->GetPathName();
		Preview->SetMesh(Candidate);
		Viewport->FocusOnMesh();
	}
}

void SAnimStitchPanel::TogglePlayback()
{
	if (bPlaying)
	{
		Pause();
	}
	else
	{
		Play();
	}
}

FReply SAnimStitchPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::G && InKeyEvent.IsAltDown() && InKeyEvent.IsShiftDown() && !InKeyEvent.IsRepeat())
	{
		Preview->SetShowGrid(!Preview->GetShowGrid());
		return FReply::Handled();
	}
	// Never steal keys from a text box (searching the Content Browser, typing a value).
	const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetKeyboardFocusedWidget();
	const bool bTyping = Focused.IsValid() && (Focused->GetType() == FName("SEditableText") || Focused->GetType() == FName("SMultiLineEditableText"));
	const bool bPlainKey = !bTyping && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsRepeat();
	// Enter or A: key the selected bone track(s) at the playhead (Sequencer style). Enter with nothing selected still zooms to fit.
	if (bPlainKey && (InKeyEvent.GetKey() == EKeys::Enter || InKeyEvent.GetKey() == EKeys::A) && Timeline->AddKeyAtPlayhead())
	{
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Enter && !InKeyEvent.IsRepeat())
	{
		Timeline->ZoomToFit();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::S && InKeyEvent.IsControlDown() && !InKeyEvent.IsRepeat())
	{
		Save();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::SpaceBar && !InKeyEvent.IsRepeat())
	{
		TogglePlayback();
		return FReply::Handled();
	}
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SAnimStitchPanel::Play()
{
	if (Asset->Timeline.Clips.Num() == 0)
	{
		return;
	}
	double RangeStart, RangeEnd;
	Asset->GetPlaybackRange(RangeStart, RangeEnd);
	if (Timeline->GetPlayhead() >= RangeEnd || Timeline->GetPlayhead() < RangeStart)
	{
		Timeline->SetPlayhead(RangeStart);
	}
	bPlaying = true;
}

void SAnimStitchPanel::Pause()
{
	bPlaying = false;
}

void SAnimStitchPanel::Stop()
{
	bPlaying = false;
	double RangeStart, RangeEnd;
	Asset->GetPlaybackRange(RangeStart, RangeEnd);
	Timeline->SetPlayhead(RangeStart);
}

FText SAnimStitchPanel::GetTimeText() const
{
	static const FNumberFormattingOptions Fmt = FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
	return FText::Format(LOCTEXT("TimeReadout", "{0} / {1} s"), FText::AsNumber(Timeline->GetPlayhead(), &Fmt), FText::AsNumber(Asset->Timeline.GetTotalLength(), &Fmt));
}

FText SAnimStitchPanel::GetProblemText() const
{
	return Preview->GetProblem();
}

EVisibility SAnimStitchPanel::GetProblemVisibility() const
{
	return Preview->GetProblem().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
}

void SAnimStitchPanel::OnMeshChanged(const FAssetData& Data)
{
	USkeletalMesh* NewMesh = Cast<USkeletalMesh>(Data.GetAsset());
	MeshPath = NewMesh ? NewMesh->GetPathName() : FString();
	Preview->SetMesh(NewMesh);
	Viewport->FocusOnMesh();

	UAnimStitchSettings* Settings = GetMutableDefault<UAnimStitchSettings>();
	Settings->LastPreviewMesh = NewMesh ? FSoftObjectPath(NewMesh) : FSoftObjectPath();
	Settings->SaveConfig();
}

bool SAnimStitchPanel::ShouldFilterMesh(const FAssetData& Data) const
{
	// Returns true to hide the asset: only meshes on the timeline's skeleton are offered.
	const USkeleton* Skeleton = Asset->GetSkeleton();
	FString SkeletonTag;
	if (!Skeleton || !Data.GetTagValue(FName("Skeleton"), SkeletonTag))
	{
		return false;
	}
	return SkeletonTag != FAssetData(Skeleton).GetExportTextName();
}

FText SAnimStitchPanel::GetStatusText() const
{
	const FAnimStitchTimeline& TL = Asset->Timeline;
	FText Info;
	if (TL.Clips.Num() == 0)
	{
		Info = LOCTEXT("StatusEmpty", "No clips.");
	}
	else
	{
		Info = FText::Format(LOCTEXT("StatusInfo", "{0} clip(s)  |  {1}s total  |  Skeleton: {2}"),
			TL.Clips.Num(), FText::AsNumber(TL.GetTotalLength(), &FNumberFormattingOptions().SetMaximumFractionalDigits(2)),
			FText::FromString(GetNameSafe(Asset->GetSkeleton())));
	}
	return LastWarning.IsEmpty() ? Info : FText::Format(LOCTEXT("StatusWithWarning", "{0}\n{1}"), Info, LastWarning);
}

void SAnimStitchPanel::HandleWarnings(const TArray<FText>& Warnings)
{
	FTextBuilder Builder;
	for (const FText& W : Warnings)
	{
		Builder.AppendLine(W);
	}
	LastWarning = Builder.ToText();
	FNotificationInfo Info(LastWarning);
	Info.ExpireDuration = 6.f;
	Info.bUseLargeFont = false;
	FSlateNotificationManager::Get().AddNotification(Info);
}

bool SAnimStitchPanel::IsCurveEnabled() const
{
	return Timeline.IsValid() && Timeline->IsOverlapSelected();
}

ECheckBoxState SAnimStitchPanel::GetCurveState(EAnimStitchBlendCurve Curve) const
{
	const int32 Index = Timeline.IsValid() ? Timeline->GetSelectedClip() : INDEX_NONE;
	return Asset->Timeline.Clips.IsValidIndex(Index) && Asset->Timeline.Clips[Index].BlendIn == Curve ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

#undef LOCTEXT_NAMESPACE


















