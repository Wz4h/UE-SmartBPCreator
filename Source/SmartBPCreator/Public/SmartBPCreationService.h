#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UClass;

/**
 * Smart BP 创建服务
 *
 * 说明：
 * 1. 这是一个纯静态服务类，不维护任何运行时状态
 * 2. V1 仅支持普通 Blueprint（BPTYPE_Normal）
 * 3. 默认命名规则：BP_类名
 * 4. 创建后不自动打开蓝图，由上层模块决定后续行为（如选中并重命名）
 */
class SMARTBPCREATOR_API FSmartBPCreationService
{
public:
	/**
	 * 根据父类和目标目录创建一个智能命名的 Blueprint
	 *
	 * @param ParentClass  蓝图父类
	 * @param TargetFolder 目标目录，例如 /Game/Blueprints
	 * @return 创建成功返回 UBlueprint*，失败返回 nullptr
	 */
	static UBlueprint* CreateSmartBlueprint(const UClass* ParentClass, const FString& TargetFolder);

private:
	/** 生成基础蓝图名称，例如 Actor -> BP_Actor */
	static FString MakeBaseBlueprintName(const UClass* ParentClass);

	/** 根据目标目录生成唯一蓝图名称，例如 BP_Actor_01 */
	static FString MakeUniqueBlueprintName(const FString& TargetFolder, const FString& BaseName);

	/** 真正执行 Blueprint 资产创建 */
	static UBlueprint* CreateBlueprintAsset(const UClass* ParentClass, const FString& TargetFolder, const FString& AssetName);

	/** 规范化目标目录 */
	static FString NormalizeTargetFolder(const FString& InFolder);
};