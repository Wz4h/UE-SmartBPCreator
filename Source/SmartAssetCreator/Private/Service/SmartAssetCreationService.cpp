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
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Rule/SmartAssetRuleResolver.h"

FSmartAssetCreationService::FSmartAssetCreationService()
{
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

UClass* FSmartAssetCreationService::GetDefaultParentClass(ESmartAssetType AssetType) const
{
	switch (AssetType)
	{
	case ESmartAssetType::ActorBlueprint:
		return UObject::StaticClass();
	case ESmartAssetType::WidgetBlueprint:
		return UUserWidget::StaticClass();
	case ESmartAssetType::AnimBlueprint:
		return UAnimInstance::StaticClass();
	case ESmartAssetType::DataAsset:
		return UDataAsset::StaticClass();
	default:
		return nullptr;
	}
}

ESmartAssetType FSmartAssetCreationService::InferAssetTypeFromBlueprint(const UBlueprint* Blueprint) const
{
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return ESmartAssetType::ActorBlueprint;
	}

	if (Blueprint->BlueprintType == BPTYPE_Interface)
	{
		return ESmartAssetType::InterfaceBlueprint;
	}

	if (Blueprint->GeneratedClass->IsChildOf(UUserWidget::StaticClass()))
	{
		return ESmartAssetType::WidgetBlueprint;
	}

	if (Blueprint->GeneratedClass->IsChildOf(UAnimInstance::StaticClass()))
	{
		return ESmartAssetType::AnimBlueprint;
	}

	return ESmartAssetType::ActorBlueprint;
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

	if (PreparedRequest.ParentBlueprint && PreparedRequest.ParentBlueprint->GeneratedClass && PreparedRequest.ParentClass == nullptr)
	{
		PreparedRequest.ParentClass = PreparedRequest.ParentBlueprint->GeneratedClass;
	}

	if (PreparedRequest.ParentBlueprint)
	{
		PreparedRequest.AssetType = InferAssetTypeFromBlueprint(PreparedRequest.ParentBlueprint);
	}

	if (PreparedRequest.ParentClass == nullptr)
	{
		PreparedRequest.ParentClass = GetDefaultParentClass(PreparedRequest.AssetType);
	}

	return PreparedRequest;
}
