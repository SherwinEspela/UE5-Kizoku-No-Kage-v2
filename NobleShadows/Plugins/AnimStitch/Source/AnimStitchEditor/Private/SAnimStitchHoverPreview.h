#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/GCObject.h"
#include "Animation/AnimSequence.h"

class FAnimStitchPreview;
class SAnimStitchViewport;
class USkeletalMesh;

/** Tooltip content for the sequence browser: a small looping 3D preview of the animation plus file info. */
class SAnimStitchHoverPreview : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAnimStitchHoverPreview) {}
		SLATE_ARGUMENT(TObjectPtr<UAnimSequence>, Sequence)
		/** Preferred preview character; falls back to the skeleton's own preview mesh. */
		SLATE_ARGUMENT(TObjectPtr<USkeletalMesh>, Mesh)
		SLATE_ARGUMENT(FString, PackageName)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SAnimStitchHoverPreview"); }

private:
	EActiveTimerReturnType Advance(double CurrentTime, float DeltaTime);

	TObjectPtr<UAnimSequence> Sequence;
	TObjectPtr<USkeletalMesh> Mesh;
	TSharedPtr<FAnimStitchPreview> Preview;
	TSharedPtr<SAnimStitchViewport> Viewport;
	double Time = 0.0;
};
