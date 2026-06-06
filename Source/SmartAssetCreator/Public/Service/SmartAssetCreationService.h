#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"

class ISmartAssetCreator;

class SMARTASSETCREATOR_API FSmartAssetCreationService
{
public:
	FSmartAssetCreationService();

	FSmartAssetCreateResult CreateAsset(const FSmartAssetCreateRequest& Request) const;
	FString BuildAssetNamePreview(const FSmartAssetCreateRequest& Request) const;
	FString ValidateRequest(const FSmartAssetCreateRequest& Request) const;

private:
	FString NormalizeTargetFolder(const FString& InFolder) const;
	FString MakeUniqueAssetName(const FString& TargetFolder, const FString& BaseName) const;
	FSmartAssetCreateRequest PrepareRequest(const FSmartAssetCreateRequest& Request) const;
	FString ValidatePreparedRequest(const FSmartAssetCreateRequest& Request) const;

private:
	TArray<TSharedRef<ISmartAssetCreator>> Creators;
};
