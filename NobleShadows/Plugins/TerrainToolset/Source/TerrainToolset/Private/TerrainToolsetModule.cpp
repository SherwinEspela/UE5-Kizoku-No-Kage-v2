#include "TerrainToolsetModule.h"

#include "TerrainToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

void FTerrainToolsetModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UTerrainToolset::StaticClass());
}

void FTerrainToolsetModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UTerrainToolset::StaticClass());
}

IMPLEMENT_MODULE(FTerrainToolsetModule, TerrainToolset)
