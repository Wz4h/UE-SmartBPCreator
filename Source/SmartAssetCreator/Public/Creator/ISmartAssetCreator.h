#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"

class ISmartAssetCreator
{
public:
	virtual ~ISmartAssetCreator() = default;

	virtual bool CanCreate(const FSmartAssetCreateRequest& Request) const = 0;
	virtual FSmartAssetCreateResult Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const = 0;
};
