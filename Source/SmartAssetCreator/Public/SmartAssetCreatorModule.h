// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"

class UBlueprint;
class UToolMenu;
class SWindow;
class SSmartAssetCreateWindow;
class FSmartAssetCreationService;
struct FSmartAssetCreateRequest;
struct FSmartAssetCreateResult;

class SMARTASSETCREATOR_API FSmartAssetCreatorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void HandleSettingsChanged();
	void PopulateAddNewMenu(UToolMenu* InMenu);
	void OpenCreateWindow(const FString& TargetFolder, UBlueprint* ParentBlueprint = nullptr);
	FString ConvertMenuPathToInternalPath(FName InMenuPath) const;
	FSmartAssetCreateResult HandleCreateRequest(const FSmartAssetCreateRequest& Request) const;
	FString HandlePreviewRequest(const FSmartAssetCreateRequest& Request) const;
	FString HandleValidationRequest(const FSmartAssetCreateRequest& Request) const;
	void HandleOpenSettingsRequest() const;
	void FocusAndRenameAsset(UObject* Asset) const;

private:
	FDelegateHandle ToolMenusStartupCallbackHandle;
	FDelegateHandle SettingsChangedHandle;
	mutable FTSTicker::FDelegateHandle PendingRenameTickerHandle;
	TUniquePtr<FSmartAssetCreationService> AssetCreationService;
	TArray<TWeakPtr<SWindow>> OpenCreateWindows;
	TArray<TWeakPtr<SSmartAssetCreateWindow>> OpenCreateWidgets;
};
