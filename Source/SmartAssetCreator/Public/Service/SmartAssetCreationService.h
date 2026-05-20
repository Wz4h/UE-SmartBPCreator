#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"

class ISmartAssetCreator;
class UBlueprint;

class SMARTASSETCREATOR_API FSmartAssetCreationService
{
public:
	FSmartAssetCreationService();

	FSmartAssetCreateResult CreateAsset(const FSmartAssetCreateRequest& Request) const;
	FString BuildAssetNamePreview(const FSmartAssetCreateRequest& Request) const;
	UClass* GetDefaultParentClass(ESmartAssetType AssetType) const;
	ESmartAssetType InferAssetTypeFromBlueprint(const UBlueprint* Blueprint) const;

private:
	FString NormalizeTargetFolder(const FString& InFolder) const;
	FString MakeUniqueAssetName(const FString& TargetFolder, const FString& BaseName) const;
	FSmartAssetCreateRequest PrepareRequest(const FSmartAssetCreateRequest& Request) const;

private:
	TArray<TSharedRef<ISmartAssetCreator>> Creators;
};
