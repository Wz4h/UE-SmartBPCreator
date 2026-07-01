// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Core/SmartCreationOption.h"
#include "SmartAssetCreateRequest.generated.h"

class UBlueprint;
class UMaterialInterface;
class UScriptStruct;
class USkeleton;

USTRUCT()
struct SMARTASSETCREATOR_API FSmartAssetCreateRequest
{
	GENERATED_BODY()

	UPROPERTY()
	ESmartUnderlyingAssetKind UnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;

	UPROPERTY()
	ESmartBlueprintTemplateKind BlueprintTemplateKind = ESmartBlueprintTemplateKind::Normal;

	UPROPERTY()
	FString PrefixOverride;

	UPROPERTY()
	bool bHasPrefixOverride = false;

	UPROPERTY()
	FString PrefixFallback;

	UPROPERTY()
	bool bHasPrefixFallback = false;

	UPROPERTY()
	FString TargetFolder = TEXT("/Game");

	UPROPERTY()
	TObjectPtr<UClass> ParentClass = nullptr;

	UPROPERTY()
	TObjectPtr<UBlueprint> ParentBlueprint = nullptr;

	UPROPERTY()
	TObjectPtr<UScriptStruct> RowStruct = nullptr;

	UPROPERTY()
	TObjectPtr<USkeleton> TargetSkeleton = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ParentMaterial = nullptr;

	UPROPERTY()
	bool bTemplateAnimBlueprint = false;
};
