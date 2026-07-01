// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

// Smart Asset Creator 插件的编辑器配置声明。
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Rule/SmartAssetPrefixRule.h"
#include "SmartAssetSettings.generated.h"

// 设置变化广播，用于刷新已经打开的创建窗口。
DECLARE_MULTICAST_DELEGATE(FOnSmartAssetSettingsChanged);

// 插件项目设置，保存命名前缀规则、子蓝图命名规则和创建窗口状态。
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Smart Asset Creator"))
class SMARTASSETCREATOR_API USmartAssetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// 初始化默认前缀规则和默认窗口尺寸。
	USmartAssetSettings();

	// 设置面板所在容器名称。
	virtual FName GetContainerName() const override;

	// 设置面板分类名称。
	virtual FName GetCategoryName() const override;

	// 设置面板 Section 名称。
	virtual FName GetSectionName() const override;

	// 设置面板显示名称。
	virtual FText GetSectionText() const override;

	// 设置面板说明文本。
	virtual FText GetSectionDescription() const override;
#if WITH_EDITOR
	// 编辑器中修改配置后广播刷新事件。
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// 获取全局设置变化广播。
	static FOnSmartAssetSettingsChanged& OnSettingsChanged();

	// DataTable 资产默认前缀。
	UPROPERTY(EditAnywhere, Config, Category = "Asset Type Prefixes", meta = (DisplayName = "Data Table Prefix", ToolTip = "Prefix used when creating Data Table assets."))
	FString DataTablePrefix;

	// Material 资产默认前缀。
	UPROPERTY(EditAnywhere, Config, Category = "Asset Type Prefixes", meta = (DisplayName = "Material Prefix", ToolTip = "Prefix used when creating Material assets."))
	FString MaterialPrefix;

	// Material Instance 资产默认前缀。
	UPROPERTY(EditAnywhere, Config, Category = "Asset Type Prefixes", meta = (DisplayName = "Material Instance Prefix", ToolTip = "Prefix used when creating Material Instance assets."))
	FString MaterialInstancePrefix;

	// 基于父类匹配的资产命名前缀规则。
	UPROPERTY(EditAnywhere, Config, Category = "Class Rules", meta = (DisplayName = "Class Prefix Rules", ToolTip = "Rules used for class-based assets such as Blueprint, Widget Blueprint, Anim Blueprint, Interface Blueprint, and Data Asset. The closest matching base class wins."))
	TArray<FSmartAssetPrefixRule> PrefixRules;

	// 创建子蓝图时是否使用父蓝图资产名作为基础名称。
	UPROPERTY(EditAnywhere, Config, Category = "Child Blueprints", meta = (DisplayName = "Use Parent Blueprint Name", ToolTip = "When enabled, child blueprints start from the parent blueprint asset name instead of the parent class name."))
	bool bUseParentBlueprintAssetNameForChildren = true;

	// 创建子蓝图时是否从父蓝图资产名移除已知前缀。
	UPROPERTY(EditAnywhere, Config, Category = "Child Blueprints", meta = (DisplayName = "Strip Known Prefix From Parent Name", ToolTip = "When enabled, known prefixes are removed from the parent blueprint asset name before generating the child asset name."))
	bool bStripKnownPrefixFromParentBlueprintName = true;

	// 创建子蓝图时追加的后缀。
	UPROPERTY(EditAnywhere, Config, Category = "Child Blueprints", meta = (DisplayName = "Child Blueprint Suffix", ToolTip = "Suffix appended when creating child blueprints from existing blueprint assets."))
	FString ChildBlueprintSuffix;

	// 创建窗口上次关闭时的尺寸，仅用于下次打开时恢复，不显示在项目设置中。
	UPROPERTY(Config)
	FVector2D CreateWindowSize;
};
