#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Rule/SmartAssetPrefixRule.h"
#include "SmartAssetSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "Smart Asset Creator"))
class SMARTASSETCREATOR_API USmartAssetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USmartAssetSettings();

	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	UPROPERTY(EditAnywhere, Config, Category = "Asset Type Prefixes", meta = (DisplayName = "Data Table Prefix", ToolTip = "Prefix used when creating Data Table assets."))
	FString DataTablePrefix;

	UPROPERTY(EditAnywhere, Config, Category = "Asset Type Prefixes", meta = (DisplayName = "Material Prefix", ToolTip = "Prefix used when creating Material assets."))
	FString MaterialPrefix;

	UPROPERTY(EditAnywhere, Config, Category = "Asset Type Prefixes", meta = (DisplayName = "Material Instance Prefix", ToolTip = "Prefix used when creating Material Instance assets."))
	FString MaterialInstancePrefix;

	UPROPERTY(EditAnywhere, Config, Category = "Class Rules", meta = (DisplayName = "Class Prefix Rules", ToolTip = "Rules used for class-based assets such as Blueprint, Widget Blueprint, Anim Blueprint, Interface Blueprint, and Data Asset. The closest matching base class wins."))
	TArray<FSmartAssetPrefixRule> PrefixRules;

	UPROPERTY(EditAnywhere, Config, Category = "Child Blueprints", meta = (DisplayName = "Use Parent Blueprint Name", ToolTip = "When enabled, child blueprints start from the parent blueprint asset name instead of the parent class name."))
	bool bUseParentBlueprintAssetNameForChildren = true;

	UPROPERTY(EditAnywhere, Config, Category = "Child Blueprints", meta = (DisplayName = "Strip Known Prefix From Parent Name", ToolTip = "When enabled, known prefixes are removed from the parent blueprint asset name before generating the child asset name."))
	bool bStripKnownPrefixFromParentBlueprintName = true;

	UPROPERTY(EditAnywhere, Config, Category = "Child Blueprints", meta = (DisplayName = "Child Blueprint Suffix", ToolTip = "Suffix appended when creating child blueprints from existing blueprint assets."))
	FString ChildBlueprintSuffix;
};
