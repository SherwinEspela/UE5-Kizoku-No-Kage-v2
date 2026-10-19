#include "SAnimStitchBatchDialog.h"
#include "AnimStitchTimelineAsset.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchBatchDialog::Show(UAnimStitchTimelineAsset* Template, TFunction<TArray<FAssetData>()> GetBrowserSelection, const FString& InitialFolder, int32 InitialFrameRate)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("BatchTitle", "AnimStitch - Batch Bake"))
		.ClientSize(FVector2D(900.0, 640.0))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TSharedRef<SAnimStitchBatchDialog> Dialog = SNew(SAnimStitchBatchDialog)
		.Template(Template)
		.GetBrowserSelection(GetBrowserSelection)
		.InitialFolder(InitialFolder)
		.InitialFrameRate(InitialFrameRate);
	Dialog->Window = Window;
	Window->SetContent(Dialog);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SAnimStitchBatchDialog::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Template);
	for (TArray<TObjectPtr<UAnimSequence>>& List : SlotClips)
	{
		Collector.AddReferencedObjects(List);
	}
}

void SAnimStitchBatchDialog::Construct(const FArguments& InArgs)
{
	Template = InArgs._Template;
	GetBrowserSelection = InArgs._GetBrowserSelection;
	Options.OutputFolder = InArgs._InitialFolder.IsEmpty() ? FString(TEXT("/Game")) : InArgs._InitialFolder;
	Options.FrameRate = InArgs._InitialFrameRate;

	FAnimStitchBatch::GetBatchSlots(Template->Timeline, SlotIndices);
	SlotClips.SetNum(SlotIndices.Num());

	TSharedRef<SVerticalBox> SlotList = SNew(SVerticalBox);
	for (int32 k = 0; k < SlotIndices.Num(); ++k)
	{
		const FAnimStitchClip& TemplateClip = Template->Timeline.Clips[SlotIndices[k]];
		SlotList->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(FAppStyle::GetFontStyle("BoldFont"))
					.Text(FText::Format(LOCTEXT("SlotHeader", "Slot {0}: replaces '{1}'"), SlotIndices[k] + 1, FText::FromString(GetNameSafe(TemplateClip.Sequence))))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[
					SNew(STextBlock).Text(this, &SAnimStitchBatchDialog::GetSlotSummary, k).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
					[
						SNew(SButton).Text(LOCTEXT("AddSelected", "Add Selected from Content Browser"))
						.ToolTipText(LOCTEXT("AddSelectedTip", "Adds the animation sequences currently selected in the panel's Content Browser"))
						.OnClicked_Lambda([this, k]() { AddSelectedToSlot(k); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(LOCTEXT("ClearSlot", "Clear"))
						.OnClicked_Lambda([this, k]() { SlotClips[k].Reset(); return FReply::Handled(); })
					]
				]
			]
		];
	}

	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FPathPickerConfig PathConfig;
	PathConfig.DefaultPath = Options.OutputFolder;
	PathConfig.bAllowContextMenu = true;
	PathConfig.OnPathSelected = FOnPathSelected::CreateLambda([this](const FString& Path) { Options.OutputFolder = Path; });

	ChildSlot
	[
		SNew(SBorder).Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).AutoWrapText(true)
				.Text(LOCTEXT("BatchIntro", "The timeline is the template. Every slot marked 'Batch Slot' is replaced by your source clips; all other slots, trims, crossfades and the root motion authority stay as they are. Clip N of every slot is baked together into output N."))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.55f).Padding(0, 0, 10, 0)
				[
					SNew(SScrollBox) + SScrollBox::Slot()[SlotList]
				]
				+ SHorizontalBox::Slot().FillWidth(0.45f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("OutputFolder", "Output folder")).Font(FAppStyle::GetFontStyle("BoldFont"))
					]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 4, 0, 8)
					[
						SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							ContentBrowser.Get().CreatePathPicker(PathConfig)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("Prefix", "Name prefix"))]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 10, 0)
						[
							SNew(SEditableTextBox).Text_Lambda([this]() { return FText::FromString(Options.Prefix); })
							.OnTextChanged_Lambda([this](const FText& T) { Options.Prefix = T.ToString(); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("Suffix", "suffix"))]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SEditableTextBox).Text_Lambda([this]() { return FText::FromString(Options.Suffix); })
							.OnTextChanged_Lambda([this](const FText& T) { Options.Suffix = T.ToString(); })
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)[SNew(STextBlock).Text(LOCTEXT("Fps", "Frame rate (0 = source)"))]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(80.f)
							[
								SNew(SSpinBox<int32>).MinValue(0).MaxValue(240)
								.Value_Lambda([this]() { return Options.FrameRate; })
								.OnValueChanged_Lambda([this](int32 V) { Options.FrameRate = V; })
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return Options.bFullLengthForBatchSlots ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { Options.bFullLengthForBatchSlots = S == ECheckBoxState::Checked; })
						[
							SNew(STextBlock).Text(LOCTEXT("FullLength", "Use each source clip's full length in batch slots (otherwise the template's trims, clamped)"))
							.AutoWrapText(true)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(STextBlock).Text(this, &SAnimStitchBatchDialog::GetPlanText).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0).HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return Options.bOverwriteExisting ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { Options.bOverwriteExisting = S == ECheckBoxState::Checked; })
						[
							SNew(STextBlock).Text(LOCTEXT("OverwriteExisting", "Overwrite existing outputs"))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(LOCTEXT("BakeAll", "Bake")).IsEnabled(this, &SAnimStitchBatchDialog::CanBake).OnClicked(this, &SAnimStitchBatchDialog::OnBake)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("Close", "Close"))
					.OnClicked_Lambda([this]() { if (TSharedPtr<SWindow> W = Window.Pin()) { W->RequestDestroyWindow(); } return FReply::Handled(); })
				]
			]
		]
	];
}

void SAnimStitchBatchDialog::AddSelectedToSlot(int32 SlotListIndex)
{
	if (!GetBrowserSelection || !SlotClips.IsValidIndex(SlotListIndex))
	{
		return;
	}
	const USkeleton* Skeleton = Template->GetSkeleton();
	int32 Skipped = 0;
	for (const FAssetData& Data : GetBrowserSelection())
	{
		UAnimSequence* Seq = Cast<UAnimSequence>(Data.GetAsset());
		if (Seq && Seq->GetSkeleton() == Skeleton)
		{
			SlotClips[SlotListIndex].Add(Seq);
		}
		else
		{
			++Skipped;
		}
	}
	LastWarning = Skipped > 0 ? FText::Format(LOCTEXT("SkippedSel", "{0} selected asset(s) were skipped (not animation sequences on the template's skeleton)."), Skipped) : FText::GetEmpty();
}

FText SAnimStitchBatchDialog::GetSlotSummary(int32 SlotListIndex) const
{
	if (!SlotClips.IsValidIndex(SlotListIndex) || SlotClips[SlotListIndex].Num() == 0)
	{
		return LOCTEXT("SlotEmpty", "No source clips yet. Select sequences in the Content Browser, then press Add Selected.");
	}
	constexpr int32 MaxShown = 10;
	FString Lines;
	for (int32 i = 0; i < SlotClips[SlotListIndex].Num() && i < MaxShown; ++i)
	{
		Lines += FString::Printf(TEXT("%d. %s\n"), i + 1, *GetNameSafe(SlotClips[SlotListIndex][i]));
	}
	if (SlotClips[SlotListIndex].Num() > MaxShown)
	{
		Lines += FString::Printf(TEXT("... and %d more"), SlotClips[SlotListIndex].Num() - MaxShown);
	}
	return FText::FromString(Lines.TrimEnd());
}

bool SAnimStitchBatchDialog::CanBake() const
{
	TArray<TArray<UAnimSequence*>> Raw;
	for (const TArray<TObjectPtr<UAnimSequence>>& List : SlotClips)
	{
		TArray<UAnimSequence*>& Out = Raw.AddDefaulted_GetRef();
		for (UAnimSequence* S : List) { Out.Add(S); }
	}
	FText Ignored;
	return FAnimStitchBatch::Validate(Template->Timeline, Raw, Ignored);
}

FText SAnimStitchBatchDialog::GetPlanText() const
{
	if (SlotIndices.Num() == 0)
	{
		return LOCTEXT("PlanNoSlots", "No clip is marked as a Batch Slot. Close this window, right-click a clip on the timeline and enable Batch Slot.");
	}
	TArray<TArray<UAnimSequence*>> Raw;
	for (const TArray<TObjectPtr<UAnimSequence>>& List : SlotClips)
	{
		TArray<UAnimSequence*>& Out = Raw.AddDefaulted_GetRef();
		for (UAnimSequence* S : List) { Out.Add(S); }
	}
	FText Error;
	if (!FAnimStitchBatch::Validate(Template->Timeline, Raw, Error))
	{
		return Error;
	}
	TArray<UAnimSequence*> FirstRow;
	for (const TArray<UAnimSequence*>& List : Raw) { FirstRow.Add(List[0]); }
	FText Plan = FText::Format(LOCTEXT("Plan", "Will create {0} sequence(s) in {1}, e.g. {2}"),
		Raw[0].Num(), FText::FromString(Options.OutputFolder), FText::FromString(FAnimStitchBatch::MakeOutputName(Options, FirstRow)));
	return LastWarning.IsEmpty() ? Plan : FText::Format(LOCTEXT("PlanWarn", "{0}\n{1}"), Plan, LastWarning);
}

FReply SAnimStitchBatchDialog::OnBake()
{
	TArray<TArray<UAnimSequence*>> Raw;
	for (const TArray<TObjectPtr<UAnimSequence>>& List : SlotClips)
	{
		TArray<UAnimSequence*>& Out = Raw.AddDefaulted_GetRef();
		for (UAnimSequence* S : List) { Out.Add(S); }
	}
	const FAnimStitchBatchResult Result = FAnimStitchBatch::Run(Template->Timeline, Raw, Options);

	FTextBuilder Summary;
	Summary.AppendLine(FText::Format(LOCTEXT("BatchDone", "Batch bake: {0} created, {1} failed."), Result.CreatedAssets.Num(), Result.Failures.Num()));
	for (int32 i = 0; i < Result.Failures.Num() && i < 5; ++i)
	{
		Summary.AppendLine(Result.Failures[i]);
	}
	FNotificationInfo Info(Summary.ToText());
	Info.ExpireDuration = 10.f;
	FSlateNotificationManager::Get().AddNotification(Info);

	if (Result.Failures.Num() == 0)
	{
		if (TSharedPtr<SWindow> W = Window.Pin()) { W->RequestDestroyWindow(); }
	}
	else
	{
		LastWarning = Summary.ToText();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
