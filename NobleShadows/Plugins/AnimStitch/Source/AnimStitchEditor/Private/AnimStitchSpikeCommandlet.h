#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AnimStitchSpikeCommandlet.generated.h"

/**
 * Phase-1 spike harness (headless): bakes two clips with a crossfade and reports root continuity.
 * -run=AnimStitchSpike -A=/Game/.. -B=/Game/.. -Overlap=0.3 -Authority=0 -Out=/Game/..
 */
UCLASS()
class UAnimStitchSpikeCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
};
