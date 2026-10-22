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

/** Page slugs of the online documentation, appended to the docs base URL. The ONLY place the docs targets are defined: fix them here when the website pages land. */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchDocsPages
{
	GENERATED_BODY()

	/** Landing page (empty = the base URL itself). */
	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString Overview;

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString QuickStart = TEXT("quick-start");

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString Timeline = TEXT("timeline");

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString Baking = TEXT("baking");

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString BatchBaking = TEXT("batch-baking");

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString FootLock = TEXT("foot-lock");

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString Corrections = TEXT("corrections");
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

	/** Online documentation: the Help menu and the Help buttons open BaseUrl + "/" + the page slug in the browser. */
	UPROPERTY(config, EditAnywhere, Category = "Help")
	FString DocsBaseUrl = TEXT("https://cybermash-interactive.ca/animstitch");

	UPROPERTY(config, EditAnywhere, Category = "Help")
	FAnimStitchDocsPages DocsPages;

	/** Timeline projects opened or saved most recently, newest first (Open Recent keeps 5). */
	UPROPERTY(config)
	TArray<FSoftObjectPath> RecentTimelines;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
