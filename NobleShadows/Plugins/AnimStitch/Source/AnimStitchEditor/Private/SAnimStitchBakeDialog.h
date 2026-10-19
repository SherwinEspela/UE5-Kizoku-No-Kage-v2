#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "AnimStitchTimeline.h"

class UAnimStitchTimelineAsset;
class SWindow;

/** Modal "Bake" window: output folder + name, frame rate, root motion authority clip, overwrite option. */
class SAnimStitchBakeDialog : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchBakeDialog) {}
		SLATE_ARGUMENT(TObjectPtr<UAnimStitchTimelineAsset>, Timeline)
		SLATE_ARGUMENT(FString, InitialFolder)
		SLATE_ARGUMENT(FString, InitialName)
		SLATE_ARGUMENT(int32, InitialFrameRate)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static void Show(UAnimStitchTimelineAsset* Timeline, const FString& InitialFolder, const FString& InitialName, int32 InitialFrameRate);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchBakeDialog"); }

private:
	FString GetOutputPath() const;
	FText GetSummaryText() const;
	FText GetSourceRateText() const;
	bool CanBake() const;
	FReply OnBake();

	TObjectPtr<UAnimStitchTimelineAsset> Timeline;
	TWeakPtr<SWindow> Window;

	FString Folder;
	FString Name;
	int32 FrameRate = 0;
	bool bOverwrite = false;

	TArray<TSharedPtr<FString>> AuthorityItems;
	TSharedPtr<FString> SelectedAuthority;
};


