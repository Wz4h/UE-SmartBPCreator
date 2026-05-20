#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;
class UBlueprint;
class UMaterialInterface;
class UScriptStruct;
class USkeleton;
class SBox;

struct FSmartAssetTypeOption
{
	ESmartAssetType AssetType = ESmartAssetType::ActorBlueprint;
	FText Label;
	TWeakObjectPtr<UClass> PresetClass = nullptr;
};

DECLARE_DELEGATE_RetVal_OneParam(FSmartAssetCreateResult, FOnSmartAssetCreateRequested, const FSmartAssetCreateRequest&);
DECLARE_DELEGATE_RetVal_OneParam(FString, FOnSmartAssetPreviewRequested, const FSmartAssetCreateRequest&);
DECLARE_DELEGATE_RetVal_OneParam(UClass*, FOnSmartAssetDefaultClassRequested, ESmartAssetType);

class SMARTASSETCREATOR_API SSmartAssetCreateWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSmartAssetCreateWindow)
		: _InitialAssetType(ESmartAssetType::ActorBlueprint)
		, _LockAssetType(false)
		, _InitialParentBlueprint(nullptr)
	{}
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(FString, TargetFolder)
		SLATE_ARGUMENT(ESmartAssetType, InitialAssetType)
		SLATE_ARGUMENT(bool, LockAssetType)
		SLATE_ARGUMENT(UBlueprint*, InitialParentBlueprint)
		SLATE_EVENT(FOnSmartAssetCreateRequested, OnCreateRequested)
		SLATE_EVENT(FOnSmartAssetPreviewRequested, OnPreviewRequested)
		SLATE_EVENT(FOnSmartAssetDefaultClassRequested, OnDefaultClassRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildAssetTypePicker();
	TSharedRef<SWidget> BuildDynamicOptions();
	TSharedRef<SWidget> BuildFooter();
	TSharedRef<SWidget> BuildClassPicker(const FText& Label);
	TSharedRef<SWidget> BuildObjectPicker(const FText& Label, UClass* AllowedClass, TFunction<FString()> GetterText, TFunction<void(const FAssetData&)> Setter);
	TSharedRef<SWidget> BuildDataTableStructPicker();
	void RebuildAssetTypeOptions();
	void RefreshDefaultsForAssetType();
	void RefreshClassViewer();
	FString GetPreviewText() const;
	FReply HandleCreateClicked();
	FReply HandleCancelClicked();
	void CloseWindow();
	FText GetAssetTypeText(ESmartAssetType AssetType) const;
	TArray<UScriptStruct*> GetAvailableRowStructs() const;
	FSmartAssetCreateRequest BuildRequest() const;
	TSharedRef<SWidget> CreateClassViewerWidget();
	UClass* GetRequiredBaseClassForSelection() const;
	ESmartAssetType ResolveAssetTypeForClass(const UClass* InClass) const;
	FText MakeClassOptionLabel(const UClass* InClass) const;
	bool IsBuiltInClassRule(const UClass* InClass) const;

private:
	TWeakPtr<SWindow> ParentWindowWeak;
	FString TargetFolder;
	ESmartAssetType SelectedAssetType = ESmartAssetType::ActorBlueprint;
	bool bLockAssetType = false;
	TObjectPtr<UBlueprint> InitialParentBlueprint = nullptr;
	TObjectPtr<UClass> SelectedParentClass = nullptr;
	TObjectPtr<UScriptStruct> SelectedRowStruct = nullptr;
	TObjectPtr<USkeleton> SelectedSkeleton = nullptr;
	TObjectPtr<UMaterialInterface> SelectedParentMaterial = nullptr;
	bool bTemplateAnimBlueprint = false;
	FOnSmartAssetCreateRequested OnCreateRequested;
	FOnSmartAssetPreviewRequested OnPreviewRequested;
	FOnSmartAssetDefaultClassRequested OnDefaultClassRequested;
	TArray<TSharedPtr<FSmartAssetTypeOption>> AssetTypeOptions;
	TSharedPtr<FSmartAssetTypeOption> SelectedAssetTypeOption;
	TArray<TSharedPtr<UScriptStruct*>> RowStructOptions;
	TSharedPtr<SBox> ClassViewerContainer;
};
