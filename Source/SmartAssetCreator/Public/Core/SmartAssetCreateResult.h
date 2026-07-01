// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SmartAssetCreateResult.generated.h"

USTRUCT()
struct SMARTASSETCREATOR_API FSmartAssetCreateResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bSucceeded = false;

	UPROPERTY()
	TObjectPtr<UObject> CreatedAsset = nullptr;

	UPROPERTY()
	FString FinalAssetName;

	UPROPERTY()
	FString ErrorMessage;
};
