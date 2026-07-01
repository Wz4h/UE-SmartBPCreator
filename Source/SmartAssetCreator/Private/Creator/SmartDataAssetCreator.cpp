// Copyright (c) 2026 Wz4h. All Rights Reserved.
#include "Creator/SmartDataAssetCreator.h"

#include "AssetToolsModule.h"
#include "Engine/DataAsset.h"
#include "Factories/DataAssetFactory.h"

bool FSmartDataAssetCreator::CanCreate(const FSmartAssetCreateRequest& Request) const
{
	return Request.UnderlyingKind == ESmartUnderlyingAssetKind::DataAsset;
}

FSmartAssetCreateResult FSmartDataAssetCreator::Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	UClass* DataAssetClass = Request.ParentClass.Get() ? Request.ParentClass.Get() : UDataAsset::StaticClass();
	if (!DataAssetClass->IsChildOf(UDataAsset::StaticClass()))
	{
		Result.ErrorMessage = TEXT("Data Asset requires a class derived from UDataAsset.");
		return Result;
	}
	if (DataAssetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		Result.ErrorMessage = TEXT("Data Asset requires a concrete, non-abstract class.");
		return Result;
	}

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = DataAssetClass;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, Request.TargetFolder, DataAssetClass, Factory))
	{
		Result.bSucceeded = true;
		Result.CreatedAsset = NewAsset;
		Result.FinalAssetName = AssetName;
	}
	else
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create Data Asset '%s'."), *AssetName);
	}

	return Result;
}
