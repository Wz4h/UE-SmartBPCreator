// Copyright (c) 2026 Wz4h. All Rights Reserved.

using UnrealBuildTool;

public class SmartAssetCreator : ModuleRules
{
	public SmartAssetCreator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"Slate",
				"SlateCore"
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
				"InputCore",
				"Projects",
				"PropertyEditor",
				"Settings",
				"StructViewer",
				"ToolMenus",
				"UMG",
				"UMGEditor",
				"UnrealEd"
			}
		);
	}
}
