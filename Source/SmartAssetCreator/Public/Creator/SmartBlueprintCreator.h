#pragma once

#include "Creator/ISmartAssetCreator.h"

class SMARTASSETCREATOR_API FSmartBlueprintCreator : public ISmartAssetCreator
{
public:
	virtual bool CanCreate(const FSmartAssetCreateRequest& Request) const override;
	virtual FSmartAssetCreateResult Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const override;

private:
	FSmartAssetCreateResult CreateClassBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const;
	FSmartAssetCreateResult CreateWidgetBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const;
	FSmartAssetCreateResult CreateAnimBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const;
	FSmartAssetCreateResult CreateInterfaceBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const;
};
