// Copyright (c) 2026 Wz4h. All Rights Reserved.
#include "Service/SmartAssetCreationService.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "Creator/ISmartAssetCreator.h"
#include "Creator/SmartBlueprintCreator.h"
#include "Creator/SmartDataAssetCreator.h"
#include "Creator/SmartDataTableCreator.h"
#include "Creator/SmartMaterialCreator.h"
#include "Creator/SmartMaterialInstanceCreator.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "DataTableEditorUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Rule/SmartAssetRuleResolver.h"
#include "Service/SmartCreationOptionBuilder.h"

FSmartAssetCreationService::FSmartAssetCreationService()
{
	FSmartAssetRuleResolver::LoadConfiguredRuleClasses();

	Creators.Add(MakeShared<FSmartBlueprintCreator>());
	Creators.Add(MakeShared<FSmartDataAssetCreator>());
	Creators.Add(MakeShared<FSmartDataTableCreator>());
	Creators.Add(MakeShared<FSmartMaterialCreator>());
	Creators.Add(MakeShared<FSmartMaterialInstanceCreator>());
}

FSmartAssetCreateResult FSmartAssetCreationService::CreateAsset(const FSmartAssetCreateRequest& Request) const
{
	FSmartAssetCreateRequest PreparedRequest = PrepareRequest(Request);
	PreparedRequest.TargetFolder = NormalizeTargetFolder(PreparedRequest.TargetFolder);

	FSmartAssetCreateResult InvalidRequestResult;
	InvalidRequestResult.ErrorMessage = ValidatePreparedRequest(PreparedRequest);
	if (!InvalidRequestResult.ErrorMessage.IsEmpty())
	{
		return InvalidRequestResult;
	}

	const FString BaseName = FSmartAssetRuleResolver::BuildAssetBaseName(PreparedRequest);
	const FString UniqueName = MakeUniqueAssetName(PreparedRequest.TargetFolder, BaseName);

	for (const TSharedRef<ISmartAssetCreator>& Creator : Creators)
	{
		if (Creator->CanCreate(PreparedRequest))
		{
			FSmartAssetCreateResult Result = Creator->Create(PreparedRequest, UniqueName);
			if (Result.bSucceeded && Result.FinalAssetName.IsEmpty())
			{
				Result.FinalAssetName = UniqueName;
			}
			return Result;
		}
	}

	FSmartAssetCreateResult Result;
	Result.ErrorMessage = TEXT("No asset creator is available for the selected asset type.");
	return Result;
}

FString FSmartAssetCreationService::BuildAssetNamePreview(const FSmartAssetCreateRequest& Request) const
{
	FSmartAssetCreateRequest PreparedRequest = PrepareRequest(Request);
	PreparedRequest.TargetFolder = NormalizeTargetFolder(PreparedRequest.TargetFolder);

	return MakeUniqueAssetName(PreparedRequest.TargetFolder, FSmartAssetRuleResolver::BuildAssetBaseName(PreparedRequest));
}

FString FSmartAssetCreationService::ValidateRequest(const FSmartAssetCreateRequest& Request) const
{
	FSmartAssetCreateRequest PreparedRequest = PrepareRequest(Request);
	PreparedRequest.TargetFolder = NormalizeTargetFolder(PreparedRequest.TargetFolder);
	return ValidatePreparedRequest(PreparedRequest);
}

FString FSmartAssetCreationService::NormalizeTargetFolder(const FString& InFolder) const
{
	FString Result = InFolder.TrimStartAndEnd();
	if (Result.IsEmpty())
	{
		Result = TEXT("/Game");
	}

	Result.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (Result.StartsWith(TEXT("/All/Plugins/")))
	{
		Result = FString::Printf(TEXT("/%s"), *Result.RightChop(13));
	}
	else if (Result.StartsWith(TEXT("/All/Game")))
	{
		Result = FString::Printf(TEXT("/Game%s"), *Result.RightChop(9));
	}
	else if (Result.StartsWith(TEXT("/All/Engine")))
	{
		Result = FString::Printf(TEXT("/Engine%s"), *Result.RightChop(11));
	}

	if (!Result.StartsWith(TEXT("/")))
	{
		Result = FString::Printf(TEXT("/Game/%s"), *Result);
	}

	while (Result.Len() > 1 && Result.EndsWith(TEXT("/")))
	{
		Result.LeftChopInline(1, EAllowShrinking::No);
	}

	return Result;
}

FString FSmartAssetCreationService::MakeUniqueAssetName(const FString& TargetFolder, const FString& BaseName) const
{
	auto IsAvailable = [&TargetFolder](const FString& CandidateName)
	{
		const FString PackageName = FString::Printf(TEXT("%s/%s"), *TargetFolder, *CandidateName);
		return !FPackageName::DoesPackageExist(PackageName) && FindPackage(nullptr, *PackageName) == nullptr;
	};

	if (IsAvailable(BaseName))
	{
		return BaseName;
	}

	for (int32 Index = 1; Index < 10000; ++Index)
	{
		const FString CandidateName = FString::Printf(TEXT("%s_%02d"), *BaseName, Index);
		if (IsAvailable(CandidateName))
		{
			return CandidateName;
		}
	}

	return FString::Printf(TEXT("%s_%s"), *BaseName, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

FSmartAssetCreateRequest FSmartAssetCreationService::PrepareRequest(const FSmartAssetCreateRequest& Request) const
{
	FSmartAssetCreateRequest PreparedRequest = Request;

	if (PreparedRequest.ParentBlueprint && PreparedRequest.ParentBlueprint->GeneratedClass)
	{
		PreparedRequest.ParentClass = PreparedRequest.ParentBlueprint->GeneratedClass;
	}

	if (PreparedRequest.ParentBlueprint)
	{
		PreparedRequest.UnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;
		PreparedRequest.BlueprintTemplateKind = FSmartCreationOptionBuilder::ResolveBlueprintTemplateKind(PreparedRequest.ParentClass);

		if (const UAnimBlueprint* ParentAnimBlueprint = Cast<UAnimBlueprint>(PreparedRequest.ParentBlueprint))
		{
			PreparedRequest.TargetSkeleton = ParentAnimBlueprint->TargetSkeleton;
			PreparedRequest.bTemplateAnimBlueprint = ParentAnimBlueprint->bIsTemplate;
		}
	}

	if (PreparedRequest.ParentClass == nullptr)
	{
		if (PreparedRequest.UnderlyingKind == ESmartUnderlyingAssetKind::Blueprint)
		{
			switch (PreparedRequest.BlueprintTemplateKind)
			{
			case ESmartBlueprintTemplateKind::Widget:
				PreparedRequest.ParentClass = UUserWidget::StaticClass();
				break;
			case ESmartBlueprintTemplateKind::Anim:
				PreparedRequest.ParentClass = UAnimInstance::StaticClass();
				break;
			case ESmartBlueprintTemplateKind::Interface:
				PreparedRequest.ParentClass = UInterface::StaticClass();
				break;
			default:
				PreparedRequest.ParentClass = UObject::StaticClass();
				break;
			}
		}
		else if (PreparedRequest.UnderlyingKind == ESmartUnderlyingAssetKind::DataAsset)
		{
			PreparedRequest.ParentClass = UDataAsset::StaticClass();
		}
	}

	if (PreparedRequest.BlueprintTemplateKind == ESmartBlueprintTemplateKind::Normal)
	{
		PreparedRequest.BlueprintTemplateKind = FSmartCreationOptionBuilder::ResolveBlueprintTemplateKind(PreparedRequest.ParentClass);
	}

	if (PreparedRequest.BlueprintTemplateKind == ESmartBlueprintTemplateKind::Interface)
	{
		PreparedRequest.ParentClass = UInterface::StaticClass();
	}

	return PreparedRequest;
}

FString FSmartAssetCreationService::ValidatePreparedRequest(const FSmartAssetCreateRequest& Request) const
{
	if (!FPackageName::IsValidLongPackageName(Request.TargetFolder, true))
	{
		return FString::Printf(TEXT("'%s' is not a valid Unreal package folder."), *Request.TargetFolder);
	}

	if (Request.TargetFolder == TEXT("/Engine") || Request.TargetFolder.StartsWith(TEXT("/Engine/")))
	{
		return TEXT("Assets cannot be created in Engine Content.");
	}

	switch (Request.UnderlyingKind)
	{
	case ESmartUnderlyingAssetKind::Blueprint:
		if (Request.ParentBlueprint)
		{
			if (Request.ParentBlueprint->BlueprintType == BPTYPE_Interface
				|| !FBlueprintEditorUtils::CanCreateChildBlueprint(Request.ParentBlueprint))
			{
				return TEXT("The selected Blueprint does not support child Blueprint creation.");
			}
		}
		if (!Request.ParentClass || !FKismetEditorUtilities::CanCreateBlueprintOfClass(Request.ParentClass))
		{
			return TEXT("Select a valid Blueprint parent class.");
		}
		if (Request.BlueprintTemplateKind == ESmartBlueprintTemplateKind::Widget
			&& !Request.ParentClass->IsChildOf(UUserWidget::StaticClass()))
		{
			return TEXT("Widget Blueprint parent class must derive from UUserWidget.");
		}
		if (Request.BlueprintTemplateKind == ESmartBlueprintTemplateKind::Anim
			&& !Request.ParentClass->IsChildOf(UAnimInstance::StaticClass()))
		{
			return TEXT("Anim Blueprint parent class must derive from UAnimInstance.");
		}
		if (Request.BlueprintTemplateKind == ESmartBlueprintTemplateKind::Anim
			&& !Request.bTemplateAnimBlueprint
			&& !Request.TargetSkeleton)
		{
			return TEXT("Select a target Skeleton or enable template Anim Blueprint creation.");
		}
		break;
	case ESmartUnderlyingAssetKind::DataAsset:
		if (!Request.ParentClass || !Request.ParentClass->IsChildOf(UDataAsset::StaticClass()))
		{
			return TEXT("Select a class derived from UDataAsset.");
		}
		if (Request.ParentClass->HasAnyClassFlags(CLASS_Abstract))
		{
			return TEXT("Select a concrete Data Asset class. Abstract classes cannot be instantiated.");
		}
		break;
	case ESmartUnderlyingAssetKind::DataTable:
		if (!Request.RowStruct || !FDataTableEditorUtils::IsValidTableStruct(Request.RowStruct))
		{
			return TEXT("Select a valid row struct for the Data Table.");
		}
		break;
	case ESmartUnderlyingAssetKind::MaterialInstance:
		if (!Request.ParentMaterial)
		{
			return TEXT("Select a parent material for the Material Instance.");
		}
		break;
	default:
		break;
	}

	const FString BaseName = FSmartAssetRuleResolver::BuildAssetBaseName(Request);
	if (BaseName.IsEmpty() || !FPackageName::IsValidObjectPath(FString::Printf(TEXT("%s/%s.%s"), *Request.TargetFolder, *BaseName, *BaseName)))
	{
		return FString::Printf(TEXT("'%s' is not a valid Unreal asset name."), *BaseName);
	}

	return FString();
}
