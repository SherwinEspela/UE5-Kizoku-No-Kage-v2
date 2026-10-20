#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "AnimStitchTimeline.h"
#include "AnimStitchProvenance.generated.h"

/** One source clip of a baked sequence, in timeline order. */
USTRUCT()
struct ANIMSTITCHEDITOR_API FAnimStitchProvenanceClip
{
	GENERATED_BODY()

	/** The source animation (never modified by AnimStitch). */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	FSoftObjectPath Source;

	/** Trim in / out points in seconds of the source animation. */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	double TrimIn = 0.0;

	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	double TrimOut = 0.0;

	/** Crossfade into this clip from the previous one: length in seconds (0 for the first clip) and curve. */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	double CrossfadeLength = 0.0;

	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	EAnimStitchBlendCurve CrossfadeCurve = EAnimStitchBlendCurve::Smoothstep;

	/** True for the clip whose root motion drives the bake. */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	bool bRootMotionAuthority = false;
};

/**
 * Provenance record stamped on every animation AnimStitch bakes: what produced it. Human readable (visible in the asset's details,
 * Animation > Asset User Data), has no effect on playback and is not loaded in game builds. The source assets are never touched.
 */
UCLASS()
class ANIMSTITCHEDITOR_API UAnimStitchProvenance : public UAssetUserData
{
	GENERATED_BODY()

public:
	/** Source clips in timeline order. */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	TArray<FAnimStitchProvenanceClip> Clips;

	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	FSoftObjectPath RootMotionAuthority;

	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	bool bFootLock = false;

	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	bool bPredictFeet = false;

	/** The timeline project asset this was baked from (empty for an unsaved timeline). */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	FSoftObjectPath SourceTimeline;

	/** ISO 8601 UTC time of the bake. */
	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	FString BakeDate;

	UPROPERTY(VisibleAnywhere, Category = "AnimStitch")
	FString AnimStitchVersion;

	/** Fills the record from a timeline. */
	void Fill(const FAnimStitchTimeline& Timeline, const FSoftObjectPath& InSourceTimeline);

	/** Multi-line human readable description (tooltip of the Baked Assets tab). */
	FText MakeSummary() const;

	virtual bool NeedsLoadForClient() const override { return false; }
	virtual bool NeedsLoadForServer() const override { return false; }
};
