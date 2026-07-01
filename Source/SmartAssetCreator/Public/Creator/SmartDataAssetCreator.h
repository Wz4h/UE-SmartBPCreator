// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

#include "Creator/ISmartAssetCreator.h"

class SMARTASSETCREATOR_API FSmartDataAssetCreator : public ISmartAssetCreator
{
public:
	virtual bool CanCreate(const FSmartAssetCreateRequest& Request) const override;
	virtual FSmartAssetCreateResult Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const override;
};
