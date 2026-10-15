#include "WidgetToolsetModule.h"

#include "WidgetToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

void FWidgetToolsetModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UWidgetToolset::StaticClass());
}

void FWidgetToolsetModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UWidgetToolset::StaticClass());
}

IMPLEMENT_MODULE(FWidgetToolsetModule, WidgetToolset)
