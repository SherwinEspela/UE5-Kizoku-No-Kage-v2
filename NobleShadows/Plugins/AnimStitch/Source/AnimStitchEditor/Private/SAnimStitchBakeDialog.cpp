#include "SAnimStitchBakeDialog.h"
#include "Dialogs/Dialogs.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchBaker.h"
#include "AnimStitchFootLock.h"
#include "AnimStitchSettings.h"
#include "Widgets/Input/SSearchableComboBox.h"
#include "ReferenceSkeleton.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchBakeDialog::Show(UAnimStitchTimelineAsset* Timeline, const FString& InitialFolder, const FString& InitialName, int32 InitialFrameRate)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("BakeTitle", "AnimStitch - Bake"))
		.ClientSize(FVector2D(620.0, 680.0))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TSharedRef<SAnimStitchBakeDialog> Dialog = SNew(SAnimStitchBakeDialog)
		.Timeline(Timeline)
		.InitialFolder(InitialFolder)
		.InitialName(InitialName)
		.InitialFrameRate(InitialFrameRate);
	Dialog->Window = Window;
	Window->SetContent(Dialog);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SAnimStitchBakeDialog::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Timeline);
}

void SAnimStitchBakeDialog::Construct(const FArguments& InArgs)
{
	Timeline = InArgs._Timeline;
	Folder = InArgs._InitialFolder.IsEmpty() ? FString(TEXT("/Game")) : InArgs._InitialFolder;
	Name = InArgs._InitialName;
	FrameRate = InArgs._InitialFrameRate;

	const FAnimStitchTimeline& TL = Timeline->Timeline;
	for (int32 i = 0; i < TL.Clips.Num(); ++i)
	{
		AuthorityItems.Add(MakeShared<FString>(FString::Printf(TEXT("%d: %s"), i + 1, *GetNameSafe(TL.Clips[i].Sequence))));
	}
	if (AuthorityItems.IsValidIndex(TL.RootAuthorityIndex))
	{
		SelectedAuthority = AuthorityItems[TL.RootAuthorityIndex];
	}

	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FPathPickerConfig PathConfig;
	PathConfig.DefaultPath = Folder;
	PathConfig.bAllowContextMenu = true;
	PathConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& Path) { Folder = Path; });

	ChildSlot
	[
		SNew(SBorder).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("OutputFolderLabel", "Output folder")).Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 4, 0, 8)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					ContentBrowser.Get().CreatePathPicker(PathConfig)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[SNew(STextBlock).Text(LOCTEXT("NameLabel", "Asset name"))]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SEditableTextBox).Text_Lambda([this]() { return FText::FromString(Name); })
					.OnTextChanged_Lambda([this](const FText& T) { Name = T.ToString(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[SNew(STextBlock).Text(LOCTEXT("FrameRateLabel", "Frame rate (0 = source)"))]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
				[
					SNew(SBox).WidthOverride(80.f)
					[
						SNew(SSpinBox<int32>).MinValue(0).MaxValue(240)
						.Value_Lambda([this]() { return FrameRate; })
						.OnValueChanged_Lambda([this](int32 V) { FrameRate = V; })
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(this, &SAnimStitchBakeDialog::GetSourceRateText).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[SNew(STextBlock).Text(LOCTEXT("AuthorityLabel", "Root motion authority"))]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&AuthorityItems)
					.InitiallySelectedItem(SelectedAuthority)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) { return SNew(STextBlock).Text(FText::FromString(*Item)); })
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
					{
						const int32 Index = AuthorityItems.IndexOfByKey(Item);
						if (Item.IsValid() && Index != INDEX_NONE)
						{
							SelectedAuthority = Item;
							const FScopedTransaction Tx(LOCTEXT("SetAuthorityTx", "Set Root Motion Authority"));
							Timeline->SetRootAuthority(Index);
						}
					})
					[
						SNew(STextBlock).Text_Lambda([this]() { return SelectedAuthority.IsValid() ? FText::FromString(*SelectedAuthority) : FText::GetEmpty(); })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return bOverwrite ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bOverwrite = S == ECheckBoxState::Checked; })
				[
					SNew(STextBlock).Text(LOCTEXT("OverwriteLabel", "Overwrite if the asset already exists"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 6)
			[
				SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text_Lambda([this]() { return Timeline->Timeline.FootLock.bEnabled ? LOCTEXT("FootLockOnInfo", "Foot lock is ON (switches and settings are in the tool's toolbar). Planted feet are pinned in this bake.") : LOCTEXT("FootLockOffInfo", "Foot lock is off (switch it on in the tool's toolbar)."); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).Text(this, &SAnimStitchBakeDialog::GetSummaryText).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton).Text(LOCTEXT("BakeButton", "Bake")).IsEnabled(this, &SAnimStitchBakeDialog::CanBake).OnClicked(this, &SAnimStitchBakeDialog::OnBake)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("CloseButton", "Close"))
					.OnClicked_Lambda([this]() { if (TSharedPtr<SWindow> W = Window.Pin()) { W->RequestDestroyWindow(); } return FReply::Handled(); })
				]
			]
		]
	];
}

FString SAnimStitchBakeDialog::GetOutputPath() const
{
	return Folder / Name;
}

FText SAnimStitchBakeDialog::GetSourceRateText() const
{
	const FAnimStitchTimeline& TL = Timeline->Timeline;
	if (TL.Clips.Num() > 0 && TL.Clips[0].Sequence && TL.Clips[0].Sequence->GetDataModel())
	{
		return FText::Format(LOCTEXT("SourceRate", "(source: {0} fps)"), FText::AsNumber(TL.Clips[0].Sequence->GetDataModel()->GetFrameRate().AsDecimal()));
	}
	return FText::GetEmpty();
}

bool SAnimStitchBakeDialog::CanBake() const
{
	return Timeline->Timeline.Clips.Num() > 0 && !Name.IsEmpty() && FPackageName::IsValidLongPackageName(GetOutputPath());
}

FText SAnimStitchBakeDialog::GetSummaryText() const
{
	if (Timeline->Timeline.Clips.Num() == 0)
	{
		return LOCTEXT("SummaryNoClips", "The timeline has no clips.");
	}
	if (Name.IsEmpty() || !FPackageName::IsValidLongPackageName(GetOutputPath()))
	{
		return LOCTEXT("SummaryBadName", "Enter a valid asset name.");
	}
	const FText Exists = FAnimStitchBaker::AssetExists(GetOutputPath()) ? LOCTEXT("SummaryExists", "  An asset with this name already exists.") : FText::GetEmpty();
	return FText::Format(LOCTEXT("Summary", "Will create {0}  ({1} s){2}"), FText::FromString(GetOutputPath()),
		FText::AsNumber(Timeline->Timeline.GetTotalLength(), &FNumberFormattingOptions().SetMaximumFractionalDigits(2)), Exists);
}

FReply SAnimStitchBakeDialog::OnBake()
{
	const FString Path = GetOutputPath();
	bool bAllowOverwrite = bOverwrite;
	if (FAnimStitchBaker::AssetExists(Path) && !bAllowOverwrite)
	{
		if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("ConfirmOverwrite", "An asset with this name already exists. Overwrite it?")) != EAppReturnType::Yes)
		{
			return FReply::Handled();
		}
		bAllowOverwrite = true;
	}

FAnimStitchBakeOptions Options;
	Options.FrameRate = FrameRate > 0 ? FFrameRate(FrameRate, 1) : FFrameRate(0, 1);

	// Only the region between the timeline's range flags is baked. A partial range needs the user's confirmation.
	{
		double RangeStart, RangeEnd;
		Timeline->GetPlaybackRange(RangeStart, RangeEnd);
		const double Total = Timeline->Timeline.GetTotalLength();
		constexpr double Eps = 1e-4;
		if (RangeStart > Eps || RangeEnd < Total - Eps)
		{
			double Fps = FrameRate;
			if (!(Fps > 0.0) && Timeline->Timeline.Clips.Num() > 0 && Timeline->Timeline.Clips[0].Sequence)
			{
				Fps = Timeline->Timeline.Clips[0].Sequence->GetDataModel()->GetFrameRate().AsDecimal();
			}
			if (!(Fps >= 1.0 && Fps <= 120.0)) { Fps = 30.0; }
			const int32 FullFrames = FMath::Max(1, FMath::RoundToInt(Total * Fps));
			const int32 First = FMath::Clamp(FMath::RoundToInt(RangeStart * Fps), 0, FullFrames - 1);
			const int32 Last = FMath::Clamp(FMath::RoundToInt(RangeEnd * Fps), First + 1, FullFrames);

			FSuppressableWarningDialog::FSetupInfo Setup(
				FText::Format(LOCTEXT("PartialBakeBody", "The timeline range flags do not cover the whole timeline.\n\nOnly the flagged section (frames {0} - {1} of {2}) will be baked, not the whole timeline. The baked sequence starts at frame {0}."), First, Last, FullFrames),
				LOCTEXT("PartialBakeTitle", "Bake Flagged Section Only"), TEXT("AnimStitchPartialBake"));
			Setup.ConfirmText = LOCTEXT("BakeAnyway", "Bake Anyway");
			Setup.CancelText = LOCTEXT("BakeCancel", "Cancel");
			Setup.bDefaultToSuppressInTheFuture = false;
			FSuppressableWarningDialog Dialog(Setup);
			if (Dialog.ShowModal() == FSuppressableWarningDialog::Cancel)
			{
				return FReply::Handled();
			}
			Options.bPartialRange = true;
			Options.RangeStart = RangeStart;
			Options.RangeEnd = RangeEnd;
		}
	}
	Options.bOverwriteExisting = bAllowOverwrite;
	Options.bShowProgress = true;
	Options.SourceTimeline = Timeline->GetOutermost() != GetTransientPackage() ? FSoftObjectPath(Timeline) : FSoftObjectPath();

	FString Error;
	UAnimSequence* Baked = FAnimStitchBaker::Bake(Timeline->Timeline, Path, Options, Error);

	if (Baked)
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("BakeDone", "Baked {0}"), FText::FromString(Baked->GetName())));
		Info.ExpireDuration = 8.f;
		Info.HyperlinkText = LOCTEXT("ShowInBrowser", "Show in Content Browser");
		const TWeakObjectPtr<UAnimSequence> WeakBaked = Baked;
		Info.Hyperlink = FSimpleDelegate::CreateLambda([WeakBaked]()
		{
			if (GEditor && WeakBaked.IsValid())
			{
				TArray<UObject*> Objects = { WeakBaked.Get() };
				GEditor->SyncBrowserToObjects(Objects);
			}
		});
		FSlateNotificationManager::Get().AddNotification(Info);
		if (TSharedPtr<SWindow> W = Window.Pin()) { W->RequestDestroyWindow(); }
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("BakeFailed", "Bake failed: {0}"), FText::FromString(Error)));
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE





