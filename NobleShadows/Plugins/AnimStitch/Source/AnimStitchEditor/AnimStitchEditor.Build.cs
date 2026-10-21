using UnrealBuildTool;

public class AnimStitchEditor : ModuleRules
{
	public AnimStitchEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"UnrealEd",
			"Slate",
			"AppFramework",
			"AppFramework",
			"SlateCore",
			"InputCore",
				"RenderCore",
			"EditorFramework",
			"WorkspaceMenuStructure",
			"AdvancedPreviewScene",
			"DeveloperSettings",
			"PropertyEditor",
			"ContentBrowser",
			"AssetTools",
			"AssetDefinition",
			"Projects",
		});
	}
}
