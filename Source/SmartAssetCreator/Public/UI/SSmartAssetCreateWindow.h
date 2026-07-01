// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

// Smart Asset Creator 创建窗口的 Slate UI 声明。
#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "Service/SmartCreationOptionBuilder.h"
#include "UObject/GCObject.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

// 创建窗口所属的外层 Slate 窗口。
class SWindow;

// 作为创建来源或父类来源的蓝图资产。
class UBlueprint;

// 材质实例创建时选择的父材质。
class UMaterialInterface;

// DataTable 创建时选择的行结构体。
class UScriptStruct;

// AnimBlueprint 创建时选择的目标骨架。
class USkeleton;

// 用于承载 ClassViewer 的容器控件。
class SBox;

// 请求创建资产的回调，返回创建结果。
DECLARE_DELEGATE_RetVal_OneParam(FSmartAssetCreateResult, FOnSmartAssetCreateRequested, const FSmartAssetCreateRequest&);

// 请求生成预览名称的回调。
DECLARE_DELEGATE_RetVal_OneParam(FString, FOnSmartAssetPreviewRequested, const FSmartAssetCreateRequest&);

// 请求校验当前创建参数的回调。
DECLARE_DELEGATE_RetVal_OneParam(FString, FOnSmartAssetValidationRequested, const FSmartAssetCreateRequest&);

// 请求打开插件设置面板的回调。
DECLARE_DELEGATE(FOnSmartAssetOpenSettingsRequested);

// 智能资产创建窗口，负责展示创建模板、动态参数、预览和创建按钮。
class SMARTASSETCREATOR_API SSmartAssetCreateWindow : public SCompoundWidget, public FGCObject
{
public:
	// Slate 构造参数。
	SLATE_BEGIN_ARGS(SSmartAssetCreateWindow)
		: _InitialUnderlyingKind(ESmartUnderlyingAssetKind::Blueprint)
		, _LockCreationOption(false)
		, _InitialParentBlueprint(nullptr)
	{}
		// 外层窗口弱引用，用于关闭窗口。
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)

		// 新资产创建的目标目录。
		SLATE_ARGUMENT(FString, TargetFolder)

		// 打开窗口时默认选择的底层资产类型。
		SLATE_ARGUMENT(ESmartUnderlyingAssetKind, InitialUnderlyingKind)

		// 是否锁定创建模板下拉框。
		SLATE_ARGUMENT(bool, LockCreationOption)

		// 从已有蓝图创建子资产时传入的父蓝图。
		SLATE_ARGUMENT(UBlueprint*, InitialParentBlueprint)

		// 点击创建按钮时触发的创建请求。
		SLATE_EVENT(FOnSmartAssetCreateRequested, OnCreateRequested)

		// 参数变化时触发的预览请求。
		SLATE_EVENT(FOnSmartAssetPreviewRequested, OnPreviewRequested)

		// 点击创建前触发的参数校验请求。
		SLATE_EVENT(FOnSmartAssetValidationRequested, OnValidationRequested)

		// 点击设置按钮时触发的打开设置请求。
		SLATE_EVENT(FOnSmartAssetOpenSettingsRequested, OnOpenSettingsRequested)
	SLATE_END_ARGS()

	// 构建窗口内容并初始化当前创建模板。
	void Construct(const FArguments& InArgs);

	// 模块关闭或窗口销毁前解绑回调，避免悬挂调用。
	void UnbindModuleCallbacks();

	// 插件设置变化后刷新模板列表和当前默认值。
	void RefreshForSettingsChange();

	// 收集 Slate 窗口持有的 UObject 引用，防止被 GC 回收。
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	// 返回 GC 引用收集器显示用名称。
	virtual FString GetReferencerName() const override;

private:
	// 构建创建模板下拉框。
	TSharedRef<SWidget> BuildCreationOptionPicker();

	// 构建随模板变化的动态参数区域。
	TSharedRef<SWidget> BuildDynamicOptions();

	// 构建底部预览、创建和取消按钮。
	TSharedRef<SWidget> BuildFooter();

	// 构建父类选择器。
	TSharedRef<SWidget> BuildClassPicker(const FText& Label);

	// 构建 UObject 资产选择器。
	TSharedRef<SWidget> BuildObjectPicker(const FText& Label, UClass* AllowedClass, TFunction<FString()> GetterText, TFunction<void(const FAssetData&)> Setter);

	// 构建 DataTable 行结构体选择器。
	TSharedRef<SWidget> BuildDataTableStructPicker();

	// 从设置和内置规则重新生成创建模板列表。
	void RebuildCreationOptions();

	// 根据当前模板刷新默认父类、骨架和父材质等参数。
	void RefreshDefaultsForCreationOption();

	// 重建 ClassViewer 内容。
	void RefreshClassViewer();

	// 标记预览文本需要重新计算。
	void InvalidatePreview();

	// 获取当前创建请求的预览名称。
	FString GetPreviewText() const;

	// 获取当前参数校验错误，空字符串表示可创建。
	FString GetValidationError() const;

	// 判断当前参数是否允许点击创建。
	bool CanCreate() const;

	// 处理创建按钮点击。
	FReply HandleCreateClicked();

	// 处理取消按钮点击。
	FReply HandleCancelClicked();

	// 处理设置按钮点击。
	FReply HandleOpenSettingsClicked();

	// 关闭外层窗口。
	void CloseWindow();

	// 将当前 UI 状态组装为创建请求。
	FSmartAssetCreateRequest BuildRequest() const;

	// 创建 ClassViewer 控件。
	TSharedRef<SWidget> CreateClassViewerWidget();

	// 获取当前模板要求的基类。
	UClass* GetRequiredBaseClassForSelection() const;

	// 判断当前模板是否需要选择父类。
	bool NeedsParentClass() const;

	// 判断当前模板是否是动画蓝图。
	bool IsAnimBlueprintOption() const;

	// 判断当前模板是否是 DataTable。
	bool IsDataTableOption() const;

	// 判断当前模板是否是材质实例。
	bool IsMaterialInstanceOption() const;

private:
	// 外层窗口弱引用，避免 Slate 窗口和内容互相强持有。
	TWeakPtr<SWindow> ParentWindowWeak;

	// 新资产创建目录。
	FString TargetFolder;

	// 打开窗口时默认匹配的底层资产类型。
	ESmartUnderlyingAssetKind InitialUnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;

	// 是否禁止用户切换创建模板。
	bool bLockCreationOption = false;

	// 从已有蓝图创建子资产时的父蓝图。
	TObjectPtr<UBlueprint> InitialParentBlueprint = nullptr;

	// 当前选择的父类。
	TObjectPtr<UClass> SelectedParentClass = nullptr;

	// 当前选择的 DataTable 行结构体。
	TObjectPtr<UScriptStruct> SelectedRowStruct = nullptr;

	// 当前选择的 AnimBlueprint 目标骨架。
	TObjectPtr<USkeleton> SelectedSkeleton = nullptr;

	// 当前选择的材质实例父材质。
	TObjectPtr<UMaterialInterface> SelectedParentMaterial = nullptr;

	// 当前动画蓝图是否作为模板动画蓝图创建。
	bool bTemplateAnimBlueprint = false;

	// 创建资产的委托。
	FOnSmartAssetCreateRequested OnCreateRequested;

	// 生成预览名称的委托。
	FOnSmartAssetPreviewRequested OnPreviewRequested;

	// 校验创建参数的委托。
	FOnSmartAssetValidationRequested OnValidationRequested;

	// 打开插件设置的委托。
	FOnSmartAssetOpenSettingsRequested OnOpenSettingsRequested;

	// 创建模板构建器。
	FSmartCreationOptionBuilder CreationOptionBuilder;

	// 当前可用的创建模板列表。
	TArray<TSharedPtr<FSmartCreationOption>> CreationOptions;

	// 当前选中的创建模板。
	TSharedPtr<FSmartCreationOption> SelectedCreationOption;

	// 创建模板下拉框控件。
	TSharedPtr<SComboBox<TSharedPtr<FSmartCreationOption>>> CreationOptionComboBox;

	// ClassViewer 的承载容器。
	TSharedPtr<SBox> ClassViewerContainer;

	// 下拉框当前显示名称，包含动态预览后的有效名称。
	FText SelectedCreationOptionDisplayName;

	// 预览缓存是否已失效。
	mutable bool bPreviewDirty = true;

	// 缓存的预览名称。
	mutable FString CachedPreviewText;

	// 上次刷新预览缓存的时间。
	mutable double CachedPreviewTime = 0.0;
};
