#pragma once

#include "CoreMinimal.h"
#include "AnimStitchTimeline.h"

/** Options for FAnimStitchBaker::Bake. */
struct ANIMSTITCHEDITOR_API FAnimStitchBakeOptions
{
	/** Output rate; 0 fps (the default) means "use the first clip's source rate". */
	FFrameRate FrameRate = FFrameRate(0, 1);
	/** Save the new asset to disk immediately. */
	bool bSavePackage = true;
	/** Replace an existing animation sequence at the output path (same skeleton) instead of failing. */
	bool bOverwriteExisting = false;
	/** Show a cancellable progress dialog (editor UI only). */
	bool bShowProgress = false;
};

/** Bakes a timeline into a single new UAnimSequence (bone transforms only in v1; notifies and curves are not carried over). */
class ANIMSTITCHEDITOR_API FAnimStitchBaker
{
public:
	/**
	 * @param PackagePath  Full asset path, e.g. /Game/Anims/AS_Merged
	 * @return The new (or overwritten) sequence, or nullptr with OutError filled.
	 */
	static UAnimSequence* Bake(const FAnimStitchTimeline& Timeline, const FString& PackagePath, const FAnimStitchBakeOptions& Options, FString& OutError);

	/** Broadcast after every successful bake (the Baked Assets tab listens). */
	static FSimpleMulticastDelegate& OnAssetBaked();

	/** True if an asset package already exists at the path. */
	static bool AssetExists(const FString& PackagePath);
};

