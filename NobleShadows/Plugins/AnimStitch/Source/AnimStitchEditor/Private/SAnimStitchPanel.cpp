#include "SAnimStitchPanel.h"
#include "SAnimStitchTimeline.h"
#include "AnimStitchPreview.h"
#include "AnimStitchSettings.h"
#include "Engine/SkeletalMesh.h"
#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
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

	auto TimelineButton = [this](const FText& Label, const FText& Tip, TFunction<void()> OnClick)
	{
		return SNew(SButton)
			.Text(Label)
			.ToolTipText(Tip)
			.IsEnabled_Lambda([this]() { return Timeline.IsValid() && Timeline->CanEditAtPlayhead(); })
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); });
	};

	ChildSlot
	[
		SNew(SSplitter).Orientation(Orient_Horizontal)
		+ SSplitter::Slot().Value(0.78f)
		[
		SNew(SSplitter).Orientation(Orient_Vertical)
		+ SSplitter::Slot().Value(0.55f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(Viewport, SAnimStitchViewport).Preview(Preview)
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.f)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(6.f)
				.Visibility(this, &SAnimStitchPanel::GetProblemVisibility)
				[
					SNew(STextBlock).Text(this, &SAnimStitchPanel::GetProblemText).AutoWrapText(true)
				]
			]
		]
		+ SSplitter::Slot().Value(0.45f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 6.f, 6.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
				[
					SNew(STextBlock).Text(LOCTEXT("PreviewMesh", "Preview mesh:"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).MaxWidth(320.f).Padding(0, 0, 12, 0)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(USkeletalMesh::StaticClass())
					.ObjectPath(this, &SAnimStitchPanel::GetMeshPath)
					.OnObjectChanged(this, &SAnimStitchPanel::OnMeshChanged)
					.OnShouldFilterAsset(this, &SAnimStitchPanel::ShouldFilterMesh)
					.AllowClear(true)
					.DisplayThumbnail(false)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
				[
					IconButton(TAttribute<const FSlateBrush*>(FAppStyle::GetBrush("Animation.Forward")), LOCTEXT("PlayTip", "Play (Space)"), [this]() { Play(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
				[
					IconButton(TAttribute<const FSlateBrush*>(FAppStyle::GetBrush("Animation.Pause")), LOCTEXT("PauseTip", "Pause (Space)"), [this]() { Pause(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					IconButton(TAttribute<const FSlateBrush*>(FAppStyle::GetBrush("Animation.Stop")), LOCTEXT("StopTip", "Stop and return to the start of the playback range"), [this]() { Stop(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
				[
					IconButton(TAttribute<const FSlateBrush*>::CreateLambda([this]() { return FAppStyle::GetBrush(bLoop ? "Animation.Loop.Enabled" : "Animation.Loop.Disabled"); }), LOCTEXT("LoopTip", "Toggle looping of the playback range"), [this]() { bLoop = !bLoop; })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(STextBlock).Text(this, &SAnimStitchPanel::GetTimeText)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
				[
					ToggleButton(LOCTEXT("Ghosts", "Ghost Joins"), LOCTEXT("GhostsTip", "Show the last pose of the outgoing clip (orange) and the first pose of the incoming clip (blue) at every join"),
						[this]() { return bShowGhosts; }, [this](bool b) { bShowGhosts = b; Preview->SetShowGhosts(b); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					ToggleButton(LOCTEXT("RootPath", "Root Path"), LOCTEXT("RootPathTip", "Draw the root bone's path across the timeline on the ground"),
						[this]() { return bShowRootPath; }, [this](bool b) { bShowRootPath = b; Preview->SetShowRootPath(b); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					SNew(SButton).Text(LOCTEXT("Duplicate", "Duplicate"))
					.ToolTipText(LOCTEXT("DuplicateTip", "Duplicate the selected clip (Ctrl+D)"))
					.OnClicked_Lambda([this]() { Timeline->DuplicateSelected(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					SNew(SButton).Text(LOCTEXT("Remove", "Remove"))
					.ToolTipText(LOCTEXT("RemoveTip", "Remove the selected clip (Delete)"))
					.OnClicked_Lambda([this]() { Timeline->DeleteSelected(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 18, 0)
				[
					TimelineButton(LOCTEXT("Split", "Split"), LOCTEXT("SplitTip", "Split the clip under the playhead into two clips"), [this]() { Timeline->SplitAtPlayhead(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					TimelineButton(LOCTEXT("TrimStart", "Trim Start"), LOCTEXT("TrimStartTip", "Cut the part of the clip before the playhead"), [this]() { Timeline->TrimStartAtPlayhead(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					TimelineButton(LOCTEXT("TrimEnd", "Trim End"), LOCTEXT("TrimEndTip", "Cut the part of the clip after the playhead"), [this]() { Timeline->TrimEndAtPlayhead(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 18, 0)
				[
					SNew(SButton).Text(LOCTEXT("ZoomFit", "Zoom to Fit"))
					.OnClicked_Lambda([this]() { Timeline->ZoomToFit(); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
				[
					SNew(STextBlock).Text(LOCTEXT("CrossfadeCurve", "Crossfade curve:"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 2, 0)
				[
					CurveButton(EAnimStitchBlendCurve::Linear, LOCTEXT("Linear", "Linear"))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					CurveButton(EAnimStitchBlendCurve::Smoothstep, LOCTEXT("Smoothstep", "Ease In/Out"))
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(6.f, 2.f)
			[
				SNew(SBorder).Padding(0)
				[
					SAssignNew(Timeline, SAnimStitchTimeline)
					.TimelineAsset(Asset)
					.OnPlayheadChanged(FSimpleDelegate::CreateSP(this, &SAnimStitchPanel::Pause)) // scrubbing stops playback
					.OnWarnings(FAnimStitchWarningsDelegate::CreateSP(this, &SAnimStitchPanel::HandleWarnings))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 4.f)
			[
				SNew(STextBlock).Text(this, &SAnimStitchPanel::GetStatusText).AutoWrapText(true)
			]
		]
		]
		+ SSplitter::Slot().Value(0.22f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 6.f, 6.f, 2.f)
			[
				SNew(STextBlock).Text(LOCTEXT("BrowserTitle", "Animation Sequences")).Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(BrowserBox, SBox)
			]
		]
	];

	RebuildSequenceBrowser();

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
	AutoSelectPreviewMesh();
	Preview->SetTimeline(Asset->Timeline);
	Timeline->SetPlayhead(Timeline->GetPlayhead()); // re-clamp to the new length
	LastPlayhead = Timeline->GetPlayhead();
	Preview->SetPlayhead(LastPlayhead);
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
	Config.InitialAssetViewType = EAssetViewType::List;
	Config.bAllowNullSelection = false;
	Config.bFocusSearchBoxWhenOpened = false;
	Config.bCanShowFolders = false;
	Config.bAllowDragging = true; // drag a sequence onto the timeline
	Config.SelectionMode = ESelectionMode::Multi;
	Config.AssetShowWarningText = LOCTEXT("BrowserEmpty", "No animation sequences use this skeleton.");

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


