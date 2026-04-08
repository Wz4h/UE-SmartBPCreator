#include "SmartBPCreationService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogSmartBPCreationService, Log, All);

UBlueprint* FSmartBPCreationService::CreateSmartBlueprint(const UClass* ParentClass, const FString& TargetFolder)
{
	if (!ParentClass)
	{
		UE_LOG(LogSmartBPCreationService, Warning, TEXT("[SmartBPCreator] ParentClass is null."));
		return nullptr;
	}

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		UE_LOG(LogSmartBPCreationService, Warning, TEXT("[SmartBPCreator] Class '%s' cannot create Blueprint."),
			*ParentClass->GetName());
		return nullptr;
	}

	const FString NormalizedFolder = NormalizeTargetFolder(TargetFolder);
	const FString BaseName = MakeBaseBlueprintName(ParentClass);
	const FString UniqueName = MakeUniqueBlueprintName(NormalizedFolder, BaseName);
	const FString PackageName = FString::Printf(TEXT("%s/%s"), *NormalizedFolder, *UniqueName);

	UBlueprint* NewBlueprint = CreateBlueprintAsset(ParentClass, PackageName, UniqueName);
	if (!NewBlueprint)
	{
		UE_LOG(LogSmartBPCreationService, Error, TEXT("[SmartBPCreator] Failed to create Blueprint. Package=%s Asset=%s"),
			*PackageName, *UniqueName);
		return nullptr;
	}

	UE_LOG(LogSmartBPCreationService, Log, TEXT("[SmartBPCreator] Blueprint created successfully: %s"),
		*NewBlueprint->GetPathName());

	return NewBlueprint;
}

FString FSmartBPCreationService::MakeBaseBlueprintName(const UClass* ParentClass)
{
	if (!ParentClass)
	{
		return TEXT("BP_Blueprint");
	}

	FString ClassName = ParentClass->GetName();
	if (ClassName.IsEmpty())
	{
		ClassName = TEXT("Blueprint");
	}

	return FString::Printf(TEXT("BP_%s"), *ClassName);
}

FString FSmartBPCreationService::MakeUniqueBlueprintName(const FString& TargetFolder, const FString& BaseName)
{
	const FString NormalizedFolder = NormalizeTargetFolder(TargetFolder);

	auto IsAssetNameAvailable = [&NormalizedFolder](const FString& CandidateName) -> bool
	{
		const FString CandidatePackageName = FString::Printf(TEXT("%s/%s"), *NormalizedFolder, *CandidateName);

		// 1. 检查磁盘/已注册包
		if (FPackageName::DoesPackageExist(CandidatePackageName))
		{
			return false;
		}

		// 2. 检查内存中是否已经有同名包
		if (FindPackage(nullptr, *CandidatePackageName) != nullptr)
		{
			return false;
		}

		return true;
	};

	if (IsAssetNameAvailable(BaseName))
	{
		return BaseName;
	}

	for (int32 Index = 1; Index < 10000; ++Index)
	{
		const FString CandidateName = FString::Printf(TEXT("%s_%02d"), *BaseName, Index);
		if (IsAssetNameAvailable(CandidateName))
		{
			return CandidateName;
		}
	}

	// 极端兜底
	return FString::Printf(TEXT("%s_%s"), *BaseName, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

UBlueprint* FSmartBPCreationService::CreateBlueprintAsset(
	const UClass* ParentClass,
	const FString& PackageName,
	const FString& AssetName)
{
	if (!ParentClass)
	{
		return nullptr;
	}

	// 如果包已经在内存中，直接拿；否则创建
	UPackage* Package = FindPackage(nullptr, *PackageName);
	if (!Package)
	{
		Package = CreatePackage(*PackageName);
	}

	if (!Package)
	{
		UE_LOG(LogSmartBPCreationService, Error, TEXT("[SmartBPCreator] Failed to create/find package: %s"), *PackageName);
		return nullptr;
	}

	Package->FullyLoad();

	// 再次保险：检查包内是否已有同名蓝图对象
	if (FindObject<UBlueprint>(Package, *AssetName))
	{
		UE_LOG(LogSmartBPCreationService, Warning, TEXT("[SmartBPCreator] Blueprint already exists in package: %s.%s"),
			*PackageName, *AssetName);
		return nullptr;
	}

	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		const_cast<UClass*>(ParentClass),
		Package,
		*AssetName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	if (!NewBlueprint)
	{
		UE_LOG(LogSmartBPCreationService, Error, TEXT("[SmartBPCreator] CreateBlueprint failed. ParentClass=%s Package=%s"),
			*ParentClass->GetName(), *PackageName);
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(NewBlueprint);
	NewBlueprint->MarkPackageDirty();
	Package->MarkPackageDirty();

	return NewBlueprint;
}

FString FSmartBPCreationService::NormalizeTargetFolder(const FString& InFolder)
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
