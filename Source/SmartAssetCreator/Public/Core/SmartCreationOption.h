#pragma once

#include "CoreMinimal.h"
#include "SmartCreationOption.generated.h"

UENUM()
enum class ESmartUnderlyingAssetKind : uint8
{
	Blueprint,
	DataAsset,
	DataTable,
	Material,
	MaterialInstance
};

UENUM()
enum class ESmartBlueprintTemplateKind : uint8
{
	Normal,
	Widget,
	Anim,
	Interface
};

USTRUCT()
struct SMARTASSETCREATOR_API FSmartCreationOption
{
	GENERATED_BODY()

	UPROPERTY()
	FText DisplayName;

	UPROPERTY()
	FText TemplateName;

	UPROPERTY()
	ESmartUnderlyingAssetKind UnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;

	UPROPERTY()
	ESmartBlueprintTemplateKind BlueprintTemplateKind = ESmartBlueprintTemplateKind::Normal;

	UPROPERTY()
	TObjectPtr<UClass> ParentClass = nullptr;

	UPROPERTY()
	FString Prefix;

	UPROPERTY()
	bool bIsUserDefined = false;

	UPROPERTY()
	bool bAllowDerivedClasses = true;
};
