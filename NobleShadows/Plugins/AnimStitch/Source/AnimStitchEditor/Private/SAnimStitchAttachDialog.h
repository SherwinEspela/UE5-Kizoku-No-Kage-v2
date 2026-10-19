#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "AnimStitchTimeline.h"

class UAnimStitchTimelineAsset;
class USkeletalMesh;
class SWindow;

/** Modal "Attach Preview Asset" window: pick a static / skeletal mesh, a socket (or bone) and an offset. Preview only, never baked. */
class SAnimStitchAttachDialog : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchAttachDialog) {}
		SLATE_ARGUMENT(TObjectPtr<UAnimStitchTimelineAsset>, Timeline)
		SLATE_ARGUMENT(TObjectPtr<USkeletalMesh>, PreviewMesh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	static void Show(UAnimStitchTimelineAsset* Timeline, USkeletalMesh* PreviewMesh);

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchAttachDialog"); }

private:
	FReply OnAttach();

	TObjectPtr<UAnimStitchTimelineAsset> Timeline;
	TObjectPtr<USkeletalMesh> PreviewMesh;
	TWeakPtr<SWindow> Window;

	FString AssetPath;
	TArray<TSharedPtr<FString>> SocketItems;
	TSharedPtr<FString> SelectedSocket;
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float Scale = 1.f;
};
