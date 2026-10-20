#include "AnimStitchProvenance.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/DateTime.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

void UAnimStitchProvenance::Fill(const FAnimStitchTimeline& Timeline, const FSoftObjectPath& InSourceTimeline)
{
	Clips.Reset();
	RootMotionAuthority.Reset();
	const int32 Authority = FMath::Clamp(Timeline.RootAuthorityIndex, 0, FMath::Max(0, Timeline.Clips.Num() - 1));
	for (int32 i = 0; i < Timeline.Clips.Num(); ++i)
	{
		const FAnimStitchClip& Clip = Timeline.Clips[i];
		FAnimStitchProvenanceClip& Entry = Clips.AddDefaulted_GetRef();
		Entry.Source = FSoftObjectPath(Clip.Sequence.Get());
		Entry.TrimIn = Clip.TrimIn;
		Entry.TrimOut = Clip.TrimOut;
		Entry.CrossfadeLength = i > 0 ? Clip.OverlapIn : 0.0;
		Entry.CrossfadeCurve = Clip.BlendIn;
		Entry.bRootMotionAuthority = i == Authority;
		if (i == Authority) { RootMotionAuthority = Entry.Source; }
	}
	bFootLock = Timeline.FootLock.bEnabled;
	bPredictFeet = Timeline.bPredictFeet;
	SourceTimeline = InSourceTimeline;
	BakeDate = FDateTime::UtcNow().ToIso8601();
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AnimStitch"));
	AnimStitchVersion = Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString(TEXT("unknown"));
}

FText UAnimStitchProvenance::MakeSummary() const
{
	FString Text = TEXT("Baked from:");
	for (int32 i = 0; i < Clips.Num(); ++i)
	{
		const FAnimStitchProvenanceClip& Clip = Clips[i];
		Text += FString::Printf(TEXT("\n%d. %s  [%.2f - %.2f s]%s"), i + 1, *Clip.Source.GetAssetName(), Clip.TrimIn, Clip.TrimOut, Clip.bRootMotionAuthority ? TEXT("  (root motion)") : TEXT(""));
		if (i > 0)
		{
			Text += FString::Printf(TEXT("\n    crossfade %.2f s, %s"), Clip.CrossfadeLength, Clip.CrossfadeCurve == EAnimStitchBlendCurve::Linear ? TEXT("linear") : TEXT("ease in/out"));
		}
	}
	Text += FString::Printf(TEXT("\nFoot lock: %s   Predict feet: %s"), bFootLock ? TEXT("on") : TEXT("off"), bPredictFeet ? TEXT("on") : TEXT("off"));
	if (SourceTimeline.IsValid()) { Text += FString::Printf(TEXT("\nTimeline: %s"), *SourceTimeline.GetAssetName()); }
	Text += FString::Printf(TEXT("\nBaked %s with AnimStitch %s"), *BakeDate, *AnimStitchVersion);
	return FText::FromString(Text);
}

#undef LOCTEXT_NAMESPACE
