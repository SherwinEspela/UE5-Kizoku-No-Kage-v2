#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimStitchBlueprintLibrary.generated.h"

class UAnimSequence;
class UAnimStitchTimelineAsset;

/** Editor-only scripting access (Editor Utility Widgets / Blueprints / Python) to the AnimStitch bake. */
UCLASS()
class ANIMSTITCHEDITOR_API UAnimStitchBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bakes a timeline asset to a single animation sequence.
	 * @param OutputAssetPath  Full asset path, e.g. /Game/Anims/AS_Merged
	 * @param FrameRate        Output fps; 0 uses the first clip's source rate
	 * @param OutError         Reason for failure (empty on success)
	 * @return The baked sequence, or null on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimStitch")
	static UAnimSequence* BakeTimeline(UAnimStitchTimelineAsset* Timeline, const FString& OutputAssetPath, int32 FrameRate, bool bOverwriteExisting, FString& OutError);
};
