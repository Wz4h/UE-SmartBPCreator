using UnrealBuildTool;

public class SmartAssetCreator : ModuleRules
{
	public SmartAssetCreator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetRegistry",
				"AssetTools",
				"ClassViewer",
				"ContentBrowser",
				"ContentBrowserData",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"InputCore",
				"Projects",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UMG",
				"UMGEditor",
				"UnrealEd"
			}
		);
	}
}
