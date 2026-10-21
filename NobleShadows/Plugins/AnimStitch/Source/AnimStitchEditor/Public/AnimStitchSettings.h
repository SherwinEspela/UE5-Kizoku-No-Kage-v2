#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchSettings.generated.h"

/** Foot bones remembered for one skeleton (used by the foot lock pass). */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchFootBones
{
	GENERATED_BODY()

	UPROPERTY(config, EditAnywhere, Category = "Foot Lock")
	FName Left;

	UPROPERTY(config, EditAnywhere, Category = "Foot Lock")
	FName Right;
};

/** Per-user AnimStitch settings (Editor Preferences > Plugins > AnimStitch). */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "AnimStitch"))
class ANIMSTITCHEDITOR_API UAnimStitchSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Skeletal mesh last used in the preview viewport. */
	UPROPERTY(config, EditAnywhere, Category = "Preview", meta = (AllowedClasses = "/Script/Engine.SkeletalMesh"))
	FSoftObjectPath LastPreviewMesh;

	/** Default content folder for baked sequences (e.g. /Game/Animations). */
	UPROPERTY(config, EditAnywhere, Category = "Bake", meta = (LongPackageName))
	FDirectoryPath DefaultOutputPath;

	/** Default bake frame rate in frames per second. 0 uses the first clip's source rate. */
	UPROPERTY(config, EditAnywhere, Category = "Bake", meta = (ClampMin = "0", ClampMax = "240"))
	int32 DefaultFrameRate = 0;

	/** Crossfade length (seconds) applied to newly created overlaps. */
	UPROPERTY(config, EditAnywhere, Category = "Timeline", meta = (ClampMin = "0.0"))
	float DefaultCrossfadeLength = 0.25f;

	/** Blend curve applied to new crossfades. */
	UPROPERTY(config, EditAnywhere, Category = "Timeline")
	EAnimStitchBlendCurve DefaultCrossfadeCurve = EAnimStitchBlendCurve::Smoothstep;

	/** Foot bones chosen in the Bake window, remembered per skeleton (key = skeleton asset path). */
	UPROPERTY(config, EditAnywhere, Category = "Foot Lock")
	TMap<FString, FAnimStitchFootBones> FootBonesBySkeleton;

	/** Named panel layouts the user saved (name -> serialized layout). */
	UPROPERTY(config)
	TMap<FString, FString> SavedLayouts;

	/** Animation sequences baked by AnimStitch (shown in the Baked Assets tab). */
	UPROPERTY(config)
	TArray<FSoftObjectPath> BakedAssets;

	/** Timeline projects opened or saved most recently, newest first (Open Recent keeps 5). */
	UPROPERTY(config)
	TArray<FSoftObjectPath> RecentTimelines;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
