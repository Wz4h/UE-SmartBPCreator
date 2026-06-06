#include "Creator/SmartDataTableCreator.h"

#include "AssetToolsModule.h"
#include "DataTableEditorUtils.h"
#include "Engine/DataTable.h"
#include "Factories/DataTableFactory.h"

bool FSmartDataTableCreator::CanCreate(const FSmartAssetCreateRequest& Request) const
{
	return Request.UnderlyingKind == ESmartUnderlyingAssetKind::DataTable;
}

FSmartAssetCreateResult FSmartDataTableCreator::Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	if (!Request.RowStruct || !FDataTableEditorUtils::IsValidTableStruct(Request.RowStruct))
	{
		Result.ErrorMessage = TEXT("Data Table requires a valid row struct.");
		return Result;
	}

	UDataTableFactory* Factory = NewObject<UDataTableFactory>();
	Factory->Struct = Request.RowStruct;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, Request.TargetFolder, UDataTable::StaticClass(), Factory))
	{
		Result.bSucceeded = true;
		Result.CreatedAsset = NewAsset;
		Result.FinalAssetName = AssetName;
	}
	else
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create Data Table '%s'."), *AssetName);
	}

	return Result;
}
