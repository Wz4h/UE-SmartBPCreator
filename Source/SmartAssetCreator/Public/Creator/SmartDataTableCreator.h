#pragma once

#include "Creator/ISmartAssetCreator.h"

class SMARTASSETCREATOR_API FSmartDataTableCreator : public ISmartAssetCreator
{
public:
	virtual bool CanCreate(const FSmartAssetCreateRequest& Request) const override;
	virtual FSmartAssetCreateResult Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const override;
};
