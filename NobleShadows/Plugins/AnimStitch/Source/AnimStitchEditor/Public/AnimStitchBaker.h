#pragma once

#include "CoreMinimal.h"
#include "AnimStitchTimeline.h"

/** Bakes a timeline into a single new UAnimSequence (bone transforms only in v1). */
class ANIMSTITCHEDITOR_API FAnimStitchBaker
{
public:
	/**
	 * @param PackagePath  Full asset path, e.g. /Game/Anims/AS_Merged
	 * @param FrameRate    Output rate; if invalid, the first clip's source rate is used.
	 * @param bSavePackage Save the new asset to disk immediately.
	 * @return The new sequence, or nullptr (OutError filled).
	 */
	static UAnimSequence* Bake(const FAnimStitchTimeline& Timeline, const FString& PackagePath, FFrameRate FrameRate, bool bSavePackage, FString& OutError);
};
