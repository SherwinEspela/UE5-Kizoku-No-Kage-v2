#include "SAnimStitchAttachedProps.h"
#include "SAnimStitchTimeline.h"
#include "AnimStitchPreview.h"
#include "AnimStitchTimelineAsset.h"
#include "ScopedTransaction.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void SAnimStitchAttachedProps::Rebuild()
{
	Items.Reset();
	if (const UAnimStitchTimelineAsset* Asset = Timeline->GetAsset())
	{
		for (int32 i = 0; i < Asset->Timeline.PreviewAttachments.Num(); ++i) { Items.Add(MakeShared<int32>(i)); }
		if (Preview->GetSelectedAttachment() >= Asset->Timeline.PreviewAttachments.Num()) { Preview->SelectAttachment(INDEX_NONE); }
	}
	if (List.IsValid()) { List->RequestListRefresh(); }
}

void SAnimStitchAttachedProps::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const UAnimStitchTimelineAsset* Asset = Timeline->GetAsset();
	if (!Asset) { return; }
	// Rows read their values live, so the list is only rebuilt when props are added / removed (a rebuild would kill a running slider drag).
	if (LastAsset.Get() != Asset || LastCount != Asset->Timeline.PreviewAttachments.Num())
	{
		LastAsset = const_cast<UAnimStitchTimelineAsset*>(Asset);
		LastEditCounter = Asset->GetEditCounter();
		LastManualCounter = Asset->GetManualCounter();
		LastCount = Asset->Timeline.PreviewAttachments.Num();
		Rebuild();
	}
}

void SAnimStitchAttachedProps::Construct(const FArguments& InArgs)
{
	Preview = InArgs._Preview;
	Timeline = InArgs._Timeline;

	auto GetAttachment = [this](int32 Index) -> const FAnimStitchPreviewAttachment*
	{
		const UAnimStitchTimelineAsset* Asset = Timeline->GetAsset();
		return Asset && Asset->Timeline.PreviewAttachments.IsValidIndex(Index) ? &Asset->Timeline.PreviewAttachments[Index] : nullptr;
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.f)
		[
			SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(LOCTEXT("NoPropsAttached", "No props attached â€” use the attach button in the toolbar"))
			.Visibility_Lambda([this]() { return Items.Num() == 0 ? EVisibility::Visible : EVisibility::Collapsed; })
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SAssignNew(List, SListView<TSharedPtr<int32>>)
			.ListItemsSource(&Items)
			.SelectionMode(ESelectionMode::Single)
			.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Item, ESelectInfo::Type) { Preview->SelectAttachment(Item.IsValid() ? *Item : INDEX_NONE); })
			.OnGenerateRow_Lambda([this, GetAttachment](TSharedPtr<int32> Item, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
			{
				const int32 Index = *Item;
				return SNew(STableRow<TSharedPtr<int32>>, Owner)
				[
					SNew(SHorizontalBox)
					// Visibility toggle (kept in the timeline asset)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 2.f)
					[
						SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(3.f))
						.ToolTipText(LOCTEXT("PropVisibilityTip", "Show / hide this prop in the viewport"))
						.OnClicked_Lambda([this, Index, GetAttachment]()
						{
							if (const FAnimStitchPreviewAttachment* A = GetAttachment(Index))
							{
								const FScopedTransaction Tx(LOCTEXT("PropVisibilityTx", "Toggle Prop Visibility"));
								Timeline->GetAsset()->SetPreviewAttachmentVisible(Index, !A->bVisible);
							}
							return FReply::Handled();
						})
						[
							SNew(SImage).Image_Lambda([GetAttachment, Index]() { const FAnimStitchPreviewAttachment* A = GetAttachment(Index); return FAppStyle::GetBrush(A && !A->bVisible ? "Icons.Hidden" : "Icons.Visible"); })
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
					// Prop name + socket
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4.f, 2.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Font(FAppStyle::GetFontStyle("BoldFont"))
							.Text_Lambda([GetAttachment, Index]() { const FAnimStitchPreviewAttachment* A = GetAttachment(Index); return A ? FText::FromString(A->Asset.GetAssetName()) : FText::GetEmpty(); })
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).ColorAndOpacity(FSlateColor::UseSubduedForeground())
							.Text_Lambda([GetAttachment, Index]() { const FAnimStitchPreviewAttachment* A = GetAttachment(Index); return A ? FText::Format(LOCTEXT("PropSocket", "Socket: {0}"), FText::FromName(A->Socket)) : FText::GetEmpty(); })
						]
					]
					// Uniform scale of this prop (stored with the attachment in the timeline asset)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 2.f)
					[
						SNew(SBox).WidthOverride(90.f)
						[
							SNew(SSpinBox<float>)
							.ToolTipText(LOCTEXT("PropScaleTip", "Scale of this prop (uniform). Drag or type a value."))
							.MinValue(0.001f).MaxValue(1000.f).MinSliderValue(0.01f).MaxSliderValue(10.f).Delta(0.01f)
							.Value_Lambda([GetAttachment, Index]() { const FAnimStitchPreviewAttachment* A = GetAttachment(Index); return A ? float(A->Scale.X) : 1.f; })
							.OnBeginSliderMovement_Lambda([this]() { ScaleTransaction = MakeShared<FScopedTransaction>(LOCTEXT("PropScaleTx", "Scale Attached Prop")); })
							.OnValueChanged_Lambda([this, Index](float V)
							{
								if (ScaleTransaction.IsValid()) { Timeline->GetAsset()->SetPreviewAttachmentScale(Index, FVector(V)); }
							})
							.OnEndSliderMovement_Lambda([this, Index](float V)
							{
								Timeline->GetAsset()->SetPreviewAttachmentScale(Index, FVector(V));
								ScaleTransaction.Reset();
							})
							.OnValueCommitted_Lambda([this, Index](float V, ETextCommit::Type)
							{
								if (ScaleTransaction.IsValid()) { return; } // a slider drag already handled it
								const UAnimStitchTimelineAsset* Current = Timeline->GetAsset();
								if (Current && Current->Timeline.PreviewAttachments.IsValidIndex(Index) && FMath::IsNearlyEqual(float(Current->Timeline.PreviewAttachments[Index].Scale.X), V)) { return; } // nothing changed (the drag already applied it)
								const FScopedTransaction Tx(LOCTEXT("PropScaleTx2", "Scale Attached Prop"));
								Timeline->GetAsset()->SetPreviewAttachmentScale(Index, FVector(V));
							})
						]
					]
					// Remove (preview-only props: no confirmation)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 2.f)
					[
						SNew(SButton).ButtonStyle(FAppStyle::Get(), "SimpleButton").ContentPadding(FMargin(3.f))
						.ToolTipText(LOCTEXT("RemovePropTip", "Remove this prop from the preview"))
						.OnClicked_Lambda([this, Index]()
						{
							const FScopedTransaction Tx(LOCTEXT("RemovePropTx", "Remove Attached Prop"));
							Timeline->GetAsset()->RemovePreviewAttachment(Index);
							return FReply::Handled();
						})
						[
							SNew(SImage).Image(FAppStyle::GetBrush("Icons.Delete")).ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
				];
			})
		]
	];
	Rebuild();
}

#undef LOCTEXT_NAMESPACE

