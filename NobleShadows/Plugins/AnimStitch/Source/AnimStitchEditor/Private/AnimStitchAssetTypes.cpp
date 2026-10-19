#include "AnimStitchAssetTypes.h"
#include "AnimStitchTimelineAsset.h"
#include "AnimStitchEditorModule.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

UAnimStitchTimelineFactory::UAnimStitchTimelineFactory()
{
	SupportedClass = UAnimStitchTimelineAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UAnimStitchTimelineFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAnimStitchTimelineAsset>(InParent, InClass, InName, Flags | RF_Transactional);
}

FText UAssetDefinition_AnimStitchTimeline::GetAssetDisplayName() const
{
	return LOCTEXT("TimelineAssetName", "AnimStitch Timeline");
}

FLinearColor UAssetDefinition_AnimStitchTimeline::GetAssetColor() const
{
	return FLinearColor(0.95f, 0.65f, 0.15f);
}

TSoftClassPtr<UObject> UAssetDefinition_AnimStitchTimeline::GetAssetClass() const
{
	return UAnimStitchTimelineAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_AnimStitchTimeline::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Animation };
	return Categories;
}

EAssetCommandResult UAssetDefinition_AnimStitchTimeline::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UAnimStitchTimelineAsset* Timeline : OpenArgs.LoadObjects<UAnimStitchTimelineAsset>())
	{
		FAnimStitchEditorModule::OpenTimelineAsset(Timeline);
		break; // one tab, one timeline
	}
	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
