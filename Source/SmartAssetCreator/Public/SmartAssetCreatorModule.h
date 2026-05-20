#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UBlueprint;
class UToolMenu;
class FSmartAssetCreationService;
struct FSmartAssetCreateRequest;
struct FSmartAssetCreateResult;
enum class ESmartAssetType : uint8;

class SMARTASSETCREATOR_API FSmartAssetCreatorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void PopulateAddNewMenu(UToolMenu* InMenu);
	void OpenCreateWindow(const FString& TargetFolder, UBlueprint* ParentBlueprint = nullptr);
	FString ConvertMenuPathToInternalPath(FName InMenuPath) const;
	FSmartAssetCreateResult HandleCreateRequest(const FSmartAssetCreateRequest& Request) const;
	FString HandlePreviewRequest(const FSmartAssetCreateRequest& Request) const;
	UClass* HandleDefaultClassRequest(ESmartAssetType AssetType) const;
	void FocusAndRenameAsset(UObject* Asset) const;

private:
	FDelegateHandle ToolMenusStartupCallbackHandle;
	TUniquePtr<FSmartAssetCreationService> AssetCreationService;
};
