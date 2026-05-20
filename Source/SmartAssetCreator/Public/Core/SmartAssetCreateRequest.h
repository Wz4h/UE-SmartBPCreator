#pragma once

#include "CoreMinimal.h"
#include "SmartAssetCreateRequest.generated.h"

class UBlueprint;
class UMaterialInterface;
class UScriptStruct;
class USkeleton;

UENUM()
enum class ESmartAssetType : uint8
{
	ActorBlueprint,
	WidgetBlueprint,
	AnimBlueprint,
	InterfaceBlueprint,
	DataAsset,
	DataTable,
	Material,
	MaterialInstance
};

USTRUCT()
struct SMARTASSETCREATOR_API FSmartAssetCreateRequest
{
	GENERATED_BODY()

	UPROPERTY()
	ESmartAssetType AssetType = ESmartAssetType::ActorBlueprint;

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
