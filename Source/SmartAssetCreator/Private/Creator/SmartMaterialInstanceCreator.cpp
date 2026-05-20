#include "Creator/SmartMaterialInstanceCreator.h"

#include "AssetToolsModule.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"

bool FSmartMaterialInstanceCreator::CanCreate(const FSmartAssetCreateRequest& Request) const
{
	return Request.AssetType == ESmartAssetType::MaterialInstance;
}

FSmartAssetCreateResult FSmartMaterialInstanceCreator::Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	if (!Request.ParentMaterial)
	{
		Result.ErrorMessage = TEXT("Material Instance requires a parent material.");
		return Result;
	}

	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = Request.ParentMaterial;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, Request.TargetFolder, UMaterialInstanceConstant::StaticClass(), Factory))
	{
		Result.bSucceeded = true;
		Result.CreatedAsset = NewAsset;
		Result.FinalAssetName = AssetName;
	}
	else
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create Material Instance '%s'."), *AssetName);
	}

	return Result;
}
