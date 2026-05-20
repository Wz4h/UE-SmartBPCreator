#include "Creator/SmartBlueprintCreator.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/Blueprint.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInterface.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace
{
	template <typename AssetClassType, typename FactoryType>
	FSmartAssetCreateResult CreateAssetWithFactory(const FString& TargetFolder, const FString& AssetName, FactoryType* Factory)
	{
		FSmartAssetCreateResult Result;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, TargetFolder, AssetClassType::StaticClass(), Factory))
		{
			Result.bSucceeded = true;
			Result.CreatedAsset = NewAsset;
			Result.FinalAssetName = AssetName;
		}
		else
		{
			Result.ErrorMessage = FString::Printf(TEXT("Failed to create asset '%s' in '%s'."), *AssetName, *TargetFolder);
		}

		return Result;
	}

	UClass* ResolveBlueprintParentClass(const FSmartAssetCreateRequest& Request)
	{
		if (Request.ParentBlueprint && Request.ParentBlueprint->GeneratedClass)
		{
			return Request.ParentBlueprint->GeneratedClass;
		}

		return Request.ParentClass;
	}
}

bool FSmartBlueprintCreator::CanCreate(const FSmartAssetCreateRequest& Request) const
{
	return Request.AssetType == ESmartAssetType::ActorBlueprint
		|| Request.AssetType == ESmartAssetType::WidgetBlueprint
		|| Request.AssetType == ESmartAssetType::AnimBlueprint
		|| Request.AssetType == ESmartAssetType::InterfaceBlueprint;
}

FSmartAssetCreateResult FSmartBlueprintCreator::Create(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	switch (Request.AssetType)
	{
	case ESmartAssetType::ActorBlueprint:
		return CreateActorBlueprint(Request, AssetName);
	case ESmartAssetType::WidgetBlueprint:
		return CreateWidgetBlueprint(Request, AssetName);
	case ESmartAssetType::AnimBlueprint:
		return CreateAnimBlueprint(Request, AssetName);
	case ESmartAssetType::InterfaceBlueprint:
		return CreateInterfaceBlueprint(Request, AssetName);
	default:
		break;
	}

	FSmartAssetCreateResult Result;
	Result.ErrorMessage = TEXT("Unsupported blueprint asset type.");
	return Result;
}

FSmartAssetCreateResult FSmartBlueprintCreator::CreateActorBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	UClass* ParentClass = ResolveBlueprintParentClass(Request);
	if (!ParentClass || !ParentClass->IsChildOf(UObject::StaticClass()))
	{
		Result.ErrorMessage = TEXT("Blueprint requires a valid parent class derived from UObject.");
		return Result;
	}

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		Result.ErrorMessage = TEXT("The selected class cannot be used to create a Blueprint.");
		return Result;
	}

	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;
	Factory->BlueprintType = BPTYPE_Normal;
	Factory->bSkipClassPicker = true;

	return CreateAssetWithFactory<UBlueprint>(Request.TargetFolder, AssetName, Factory);
}

FSmartAssetCreateResult FSmartBlueprintCreator::CreateWidgetBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	UClass* ParentClass = ResolveBlueprintParentClass(Request);
	if (!ParentClass || !ParentClass->IsChildOf(UUserWidget::StaticClass()))
	{
		Result.ErrorMessage = TEXT("Widget Blueprint requires a parent class derived from UUserWidget.");
		return Result;
	}

	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = ParentClass;
	Factory->BlueprintType = BPTYPE_Normal;

	return CreateAssetWithFactory<UWidgetBlueprint>(Request.TargetFolder, AssetName, Factory);
}

FSmartAssetCreateResult FSmartBlueprintCreator::CreateAnimBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	FSmartAssetCreateResult Result;
	UClass* ParentClass = ResolveBlueprintParentClass(Request);
	if (!ParentClass || !ParentClass->IsChildOf(UAnimInstance::StaticClass()))
	{
		Result.ErrorMessage = TEXT("Anim Blueprint requires a parent class derived from UAnimInstance.");
		return Result;
	}

	UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->ParentClass = ParentClass;
	Factory->BlueprintType = BPTYPE_Normal;
	Factory->TargetSkeleton = Request.TargetSkeleton;
	Factory->bTemplate = Request.bTemplateAnimBlueprint || Request.TargetSkeleton == nullptr;

	return CreateAssetWithFactory<UAnimBlueprint>(Request.TargetFolder, AssetName, Factory);
}

FSmartAssetCreateResult FSmartBlueprintCreator::CreateInterfaceBlueprint(const FSmartAssetCreateRequest& Request, const FString& AssetName) const
{
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = UInterface::StaticClass();
	Factory->BlueprintType = BPTYPE_Interface;
	Factory->bSkipClassPicker = true;

	return CreateAssetWithFactory<UBlueprint>(Request.TargetFolder, AssetName, Factory);
}
