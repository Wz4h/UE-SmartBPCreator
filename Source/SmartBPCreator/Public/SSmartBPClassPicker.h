#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UClass;
class SWindow;

/** 选择父类后的回调 */
DECLARE_DELEGATE_OneParam(FOnBPClassPicked, UClass*);

/**
 * Blueprint 父类选择器
 *
 * 作用：
 * 1. 显示 UE 自带的 Class Viewer
 * 2. 允许用户选择一个可用于创建 Blueprint 的父类
 * 3. 点击 Confirm 后通过回调返回 UClass*
 * 4. 点击 Cancel 或 Confirm 后关闭所属窗口
 *
 * 说明：
 * - 该类只负责“选类”
 * - 不负责创建蓝图
 * - 不负责命名
 * - 不负责路径处理
 */
class SMARTBPCREATOR_API SSmartBPClassPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSmartBPClassPicker)
		: _ParentWindow()
	{}
	/** 外部创建的宿主窗口，用于 Confirm / Cancel 后关闭 */
	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)

	/** 选择完成后的回调 */
	SLATE_EVENT(FOnBPClassPicked, OnClassPicked)
SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

private:
	/** 创建 Class Viewer 控件 */
	TSharedRef<SWidget> BuildClassViewer();

	/** 用户在 Class Viewer 中选中类时回调 */
	void HandleClassPicked(UClass* InClass);

	/** 点击 Confirm */
	FReply OnConfirmClicked();

	/** 点击 Cancel */
	FReply OnCancelClicked();

	/** 关闭宿主窗口 */
	void CloseOwnerWindow();

	/** 当前选中的父类 */
	UClass* SelectedClass = nullptr;

	/** 外部回调：把选中的类传出去 */
	FOnBPClassPicked OnClassPickedDelegate;

	/** 宿主窗口 */
	TWeakPtr<SWindow> ParentWindowWeak;
};