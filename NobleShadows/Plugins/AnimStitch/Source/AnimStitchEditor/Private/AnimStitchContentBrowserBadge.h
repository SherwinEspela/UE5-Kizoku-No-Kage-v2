#pragma once

#include "CoreMinimal.h"

/**
 * The "On the AnimStitch timeline" badge on Content Browser tiles and list rows of animation sequences. It is drawn through the Content
 * Browser's extra-state extender (never by touching the asset or its thumbnail) and follows the timeline live: it appears when a clip
 * lands on the timeline, clears when it leaves, and shows a count when the same clip is used more than once.
 */
class FAnimStitchContentBrowserBadge
{
public:
	static void Register();
	static void Unregister();

	/** Called by the open AnimStitch panel whenever its timeline changes: how many clips use each sequence. */
	static void SetClipUsage(const TMap<FSoftObjectPath, int32>& Usage);
	static void ClearClipUsage();
	static int32 GetClipUsage(const FSoftObjectPath& Path);

private:
	static FDelegateHandle GeneratorHandle;
	static TMap<FSoftObjectPath, int32> ClipUsage;
};
