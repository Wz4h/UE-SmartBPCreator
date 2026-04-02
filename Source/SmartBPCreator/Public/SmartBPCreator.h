#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UClass;
class UToolMenu;

/**
 * SmartBPCreator 插件模块
 *
 * 作用：
 * 1. 扩展 Content Browser 空白区域（Add New）菜单
 * 2. 在当前目录添加“Create Blueprint (Auto Name)”入口
 * 3. 弹出父类选择窗口
 * 4. 创建 Blueprint 并自动选中 + 重命名
 *
 * 架构：
 * Module -> Picker -> Service
 */
class SMARTBPCREATOR_API FSmartBPCreatorModule : public IModuleInterface
{
public:
	/** IModuleInterface */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 注册菜单 */
	void RegisterMenus();

	/** 填充菜单内容 */
	void PopulateAddNewMenu(UToolMenu* InMenu);

	/** 点击菜单 */
	void OnCreateSmartBlueprintClicked(FName TargetPath);

	/** 打开类选择窗口 */
	void OpenClassPickerWindow(const FString& TargetPath);

	/** 处理选择结果 */
	void HandleClassPicked(UClass* PickedClass, FString TargetPath);

	/** 将菜单路径转换为内部路径（关键） */
	FString ConvertMenuPathToInternalPath(FName InMenuPath) const;

private:
	/** ToolMenus 启动句柄 */
	FDelegateHandle ToolMenusStartupCallbackHandle;
};