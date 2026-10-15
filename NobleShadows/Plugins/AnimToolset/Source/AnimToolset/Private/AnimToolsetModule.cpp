#include "AnimToolsetModule.h"

#include "AnimToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

void FAnimToolsetModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UAnimToolset::StaticClass());
}

void FAnimToolsetModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UAnimToolset::StaticClass());
}

IMPLEMENT_MODULE(FAnimToolsetModule, AnimToolset)
