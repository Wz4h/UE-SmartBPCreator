// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SmartBPCreator : ModuleRules
{
	public SmartBPCreator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Projects",
				"Slate",
				"SlateCore",
				"AssetRegistry",
				"ClassViewer",
				"ContentBrowser",
				"ContentBrowserData",
				"ToolMenus",
				"UnrealEd"
			}
		);
	}
}
