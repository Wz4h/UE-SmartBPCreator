#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "Service/SmartCreationOptionBuilder.h"
#include "UObject/GCObject.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

class SWindow;
class UBlueprint;
class UMaterialInterface;
class UScriptStruct;
class USkeleton;
class SBox;

DECLARE_DELEGATE_RetVal_OneParam(FSmartAssetCreateResult, FOnSmartAssetCreateRequested, const FSmartAssetCreateRequest&);
DECLARE_DELEGATE_RetVal_OneParam(FString, FOnSmartAssetPreviewRequested, const FSmartAssetCreateRequest&);
DECLARE_DELEGATE_RetVal_OneParam(FString, FOnSmartAssetValidationRequested, const FSmartAssetCreateRequest&);
DECLARE_DELEGATE(FOnSmartAssetOpenSettingsRequested);

class SMARTASSETCREATOR_API SSmartAssetCreateWindow : public SCompoundWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SSmartAssetCreateWindow)
		: _InitialUnderlyingKind(ESmartUnderlyingAssetKind::Blueprint)
		, _LockCreationOption(false)
		, _InitialParentBlueprint(nullptr)
	{}
		SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)
		SLATE_ARGUMENT(FString, TargetFolder)
		SLATE_ARGUMENT(ESmartUnderlyingAssetKind, InitialUnderlyingKind)
		SLATE_ARGUMENT(bool, LockCreationOption)
		SLATE_ARGUMENT(UBlueprint*, InitialParentBlueprint)
		SLATE_EVENT(FOnSmartAssetCreateRequested, OnCreateRequested)
		SLATE_EVENT(FOnSmartAssetPreviewRequested, OnPreviewRequested)
		SLATE_EVENT(FOnSmartAssetValidationRequested, OnValidationRequested)
		SLATE_EVENT(FOnSmartAssetOpenSettingsRequested, OnOpenSettingsRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void UnbindModuleCallbacks();
	void RefreshForSettingsChange();
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	TSharedRef<SWidget> BuildCreationOptionPicker();
	TSharedRef<SWidget> BuildDynamicOptions();
	TSharedRef<SWidget> BuildFooter();
	TSharedRef<SWidget> BuildClassPicker(const FText& Label);
	TSharedRef<SWidget> BuildObjectPicker(const FText& Label, UClass* AllowedClass, TFunction<FString()> GetterText, TFunction<void(const FAssetData&)> Setter);
	TSharedRef<SWidget> BuildDataTableStructPicker();
	void RebuildCreationOptions();
	void RefreshDefaultsForCreationOption();
	void RefreshClassViewer();
	void EnsureWindowSizeForCurrentOption() const;
	void InvalidatePreview();
	FString GetPreviewText() const;
	FString GetValidationError() const;
	bool CanCreate() const;
	FReply HandleCreateClicked();
	FReply HandleCancelClicked();
	FReply HandleOpenSettingsClicked();
	void CloseWindow();
	FSmartAssetCreateRequest BuildRequest() const;
	TSharedRef<SWidget> CreateClassViewerWidget();
	UClass* GetRequiredBaseClassForSelection() const;
	bool NeedsParentClass() const;
	bool IsAnimBlueprintOption() const;
	bool IsDataTableOption() const;
	bool IsMaterialInstanceOption() const;

private:
	TWeakPtr<SWindow> ParentWindowWeak;
	FString TargetFolder;
	ESmartUnderlyingAssetKind InitialUnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;
	bool bLockCreationOption = false;
	TObjectPtr<UBlueprint> InitialParentBlueprint = nullptr;
	TObjectPtr<UClass> SelectedParentClass = nullptr;
	TObjectPtr<UScriptStruct> SelectedRowStruct = nullptr;
	TObjectPtr<USkeleton> SelectedSkeleton = nullptr;
	TObjectPtr<UMaterialInterface> SelectedParentMaterial = nullptr;
	bool bTemplateAnimBlueprint = false;
	FOnSmartAssetCreateRequested OnCreateRequested;
	FOnSmartAssetPreviewRequested OnPreviewRequested;
	FOnSmartAssetValidationRequested OnValidationRequested;
	FOnSmartAssetOpenSettingsRequested OnOpenSettingsRequested;
	FSmartCreationOptionBuilder CreationOptionBuilder;
	TArray<TSharedPtr<FSmartCreationOption>> CreationOptions;
	TSharedPtr<FSmartCreationOption> SelectedCreationOption;
	TSharedPtr<SComboBox<TSharedPtr<FSmartCreationOption>>> CreationOptionComboBox;
	TSharedPtr<SBox> ClassViewerContainer;
	FText SelectedCreationOptionDisplayName;
	mutable bool bPreviewDirty = true;
	mutable FString CachedPreviewText;
	mutable double CachedPreviewTime = 0.0;
};
