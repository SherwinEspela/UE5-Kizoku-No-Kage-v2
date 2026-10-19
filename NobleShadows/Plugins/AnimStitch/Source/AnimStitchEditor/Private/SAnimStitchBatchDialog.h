#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "AnimStitchBatch.h"
#include "AssetRegistry/AssetData.h"

class UAnimStitchTimelineAsset;
class SWindow;

/**
 * Modal "Batch Bake" window. The template is the timeline asset; the user fills every batch slot with
 * source clips (taken from the Content Browser selection) and picks an output folder.
 */
class SAnimStitchBatchDialog : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchBatchDialog) {}
		SLATE_ARGUMENT(TObjectPtr<UAnimStitchTimelineAsset>, Template)
		SLATE_ARGUMENT(TFunction<TArray<FAssetData>()>, GetBrowserSelection)
		SLATE_ARGUMENT(FString, InitialFolder)
		SLATE_ARGUMENT(int32, InitialFrameRate)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Opens the modal window. */
	static void Show(UAnimStitchTimelineAsset* Template, TFunction<TArray<FAssetData>()> GetBrowserSelection, const FString& InitialFolder, int32 InitialFrameRate);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchBatchDialog"); }

private:
	void AddSelectedToSlot(int32 SlotListIndex);
	FText GetSlotSummary(int32 SlotListIndex) const;
	FText GetPlanText() const;
	bool CanBake() const;
	FReply OnBake();

	TObjectPtr<UAnimStitchTimelineAsset> Template;
	TFunction<TArray<FAssetData>()> GetBrowserSelection;
	TWeakPtr<SWindow> Window;

	TArray<int32> SlotIndices;
	/** One list of source clips per batch slot. */
	TArray<TArray<TObjectPtr<UAnimSequence>>> SlotClips;

	FAnimStitchBatchOptions Options;
	FText LastWarning;
};
