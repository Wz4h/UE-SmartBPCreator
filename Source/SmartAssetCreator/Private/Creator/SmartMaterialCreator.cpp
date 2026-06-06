#include "Creator/SmartMaterialCreator.h"

#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/Material.h"

bool FSmartMaterialCreator::CanCreate(const FSmartAssetCreateRequest& Request) const
{
	return Request.UnderlyingKind == ESmartUnderlyingAssetKind::Material;
}

FSmartAssetCreateResult FSmartMaterialCreator::Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, Request.TargetFolder, UMaterial::StaticClass(), Factory))
	{
		Result.bSucceeded = true;
		Result.CreatedAsset = NewAsset;
		Result.FinalAssetName = AssetName;
	}
	else
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create Material '%s'."), *AssetName);
	}

	return Result;
}
