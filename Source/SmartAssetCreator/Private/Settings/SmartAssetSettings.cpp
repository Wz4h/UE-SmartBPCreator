#include "Settings/SmartAssetSettings.h"

USmartAssetSettings::USmartAssetSettings()
{
	DataTablePrefix = TEXT("DT_");
	MaterialPrefix = TEXT("M_");
	MaterialInstancePrefix = TEXT("MI_");
	ChildBlueprintSuffix = TEXT("_Child");

	if (PrefixRules.IsEmpty())
	{
		FSmartAssetPrefixRule WidgetRule;
		WidgetRule.BaseClass = FSoftClassPath(TEXT("/Script/UMG.UserWidget"));
		WidgetRule.Prefix = TEXT("WBP_");

		FSmartAssetPrefixRule AnimRule;
		AnimRule.BaseClass = FSoftClassPath(TEXT("/Script/Engine.AnimInstance"));
		AnimRule.Prefix = TEXT("ABP_");

		FSmartAssetPrefixRule ObjectRule;
		ObjectRule.BaseClass = FSoftClassPath(TEXT("/Script/CoreUObject.Object"));
		ObjectRule.Prefix = TEXT("BP_");

		FSmartAssetPrefixRule InterfaceRule;
		InterfaceRule.BaseClass = FSoftClassPath(TEXT("/Script/CoreUObject.Interface"));
		InterfaceRule.Prefix = TEXT("BPI_");

		FSmartAssetPrefixRule DataAssetRule;
		DataAssetRule.BaseClass = FSoftClassPath(TEXT("/Script/Engine.DataAsset"));
		DataAssetRule.Prefix = TEXT("DA_");

		PrefixRules = { WidgetRule, AnimRule, ObjectRule, InterfaceRule, DataAssetRule };
	}
}

FName USmartAssetSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName USmartAssetSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName USmartAssetSettings::GetSectionName() const
{
	return TEXT("SmartAssetCreator");
}

FText USmartAssetSettings::GetSectionText() const
{
	return NSLOCTEXT("SmartAssetCreator", "SettingsSectionText", "Smart Asset Creator");
}

FText USmartAssetSettings::GetSectionDescription() const
{
	return NSLOCTEXT(
		"SmartAssetCreator",
		"SettingsSectionDescription",
		"Configure class prefix rules for class-based assets, fixed prefixes for non-class asset types, and child blueprint naming rules."
	);
}
