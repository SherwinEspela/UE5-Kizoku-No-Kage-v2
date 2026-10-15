using UnrealBuildTool;

public class TerrainToolset : ModuleRules
{
	public TerrainToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ToolsetRegistry",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Landscape",
			"ImageWrapper",
			"Foliage",
			"UnrealEd",
			"Json",
			"JsonUtilities",
		});
	}
}
