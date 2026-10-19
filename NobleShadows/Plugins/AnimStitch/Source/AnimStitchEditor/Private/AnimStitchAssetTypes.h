#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetDefinitionDefault.h"
#include "AnimStitchAssetTypes.generated.h"

/** Content Browser > Add > Animation > AnimStitch Timeline. */
UCLASS()
class UAnimStitchTimelineFactory : public UFactory
{
	GENERATED_BODY()

public:
	UAnimStitchTimelineFactory();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

/** Makes timeline assets open in the AnimStitch tab on double-click. */
UCLASS()
class UAssetDefinition_AnimStitchTimeline : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};
