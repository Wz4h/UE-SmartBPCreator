// Copyright (c) 2026 Wz4h. All Rights Reserved.
#include "Rule/SmartAssetRuleResolver.h"

#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Settings/SmartAssetSettings.h"

namespace
{
	FString GetClassStem(const UClass* InClass)
	{
		if (!InClass)
		{
			return TEXT("Asset");
		}

		FString ClassName = InClass->GetName();
		if (ClassName.EndsWith(TEXT("_C")))
		{
			ClassName.LeftChopInline(2, EAllowShrinking::No);
		}

		return ClassName;
	}

	FString GetStructStem(const UScriptStruct* InStruct)
	{
		if (!InStruct)
		{
			return TEXT("DataTable");
		}

		FString StructName = InStruct->GetName();
		if (StructName.StartsWith(TEXT("F")) && StructName.Len() > 1)
		{
			StructName.RightChopInline(1, EAllowShrinking::No);
		}

		return StructName;
	}
}

const USmartAssetSettings* FSmartAssetRuleResolver::GetSettings()
{
	return GetDefault<USmartAssetSettings>();
}

void FSmartAssetRuleResolver::LoadConfiguredRuleClasses()
{
	const USmartAssetSettings* Settings = GetSettings();
	if (!Settings)
	{
		return;
	}

	for (const FSmartAssetPrefixRule& Rule : Settings->PrefixRules)
	{
		if (Rule.bEnabled && !Rule.BaseClass.IsNull())
		{
			Rule.BaseClass.TryLoadClass<UObject>();
		}
	}
}

FString FSmartAssetRuleResolver::ResolvePrefix(const FSmartAssetCreateRequest& Request)
{
	if (Request.bHasPrefixOverride)
	{
		return Request.PrefixOverride;
	}

	const FString FallbackPrefix = Request.bHasPrefixFallback ? Request.PrefixFallback : ResolveBuiltInPrefix(Request);
	if ((Request.UnderlyingKind == ESmartUnderlyingAssetKind::Blueprint
			|| Request.UnderlyingKind == ESmartUnderlyingAssetKind::DataAsset)
		&& Request.ParentClass)
	{
		return ResolveClassPrefix(Request.ParentClass, FallbackPrefix);
	}

	return FallbackPrefix;
}

FString FSmartAssetRuleResolver::ResolveClassPrefix(const UClass* ParentClass, const FString& FallbackPrefix)
{
	const USmartAssetSettings* Settings = GetSettings();
	int32 BestDistance = TNumericLimits<int32>::Max();
	const FSmartAssetPrefixRule* BestRule = nullptr;

	if (Settings && ParentClass)
	{
		for (const FSmartAssetPrefixRule& Rule : Settings->PrefixRules)
		{
			if (!Rule.bEnabled)
			{
				continue;
			}

			UClass* RuleClass = Rule.BaseClass.ResolveClass();
			if (!RuleClass)
			{
				continue;
			}

			int32 Distance = INDEX_NONE;
			if (Rule.bIncludeDerivedClasses)
			{
				int32 CandidateDistance = 0;
				for (const UClass* Current = ParentClass; Current; Current = Current->GetSuperClass(), ++CandidateDistance)
				{
					if (Current == RuleClass)
					{
						Distance = CandidateDistance;
						break;
					}
				}
			}
			else if (ParentClass == RuleClass)
			{
				Distance = 0;
			}

			if (Distance != INDEX_NONE && Distance < BestDistance)
			{
				BestDistance = Distance;
				BestRule = &Rule;
			}
		}
	}

	return BestRule ? BestRule->Prefix : FallbackPrefix;
}

FString FSmartAssetRuleResolver::BuildAssetBaseName(const FSmartAssetCreateRequest& Request)
{
	const USmartAssetSettings* Settings = GetSettings();
	const FString Prefix = ResolvePrefix(Request);
	const FString Stem = BuildNameStem(Request);

	const bool bIsChildBlueprint = Request.ParentBlueprint != nullptr
		&& Request.UnderlyingKind == ESmartUnderlyingAssetKind::Blueprint;

	if (bIsChildBlueprint && Settings && !Settings->ChildBlueprintSuffix.IsEmpty())
	{
		return Prefix + Stem + Settings->ChildBlueprintSuffix;
	}

	return Prefix + Stem;
}

FString FSmartAssetRuleResolver::StripKnownPrefix(const FString& InName)
{
	const USmartAssetSettings* Settings = GetSettings();
	TArray<FString> Prefixes;

	if (Settings)
	{
		Prefixes.Add(Settings->DataTablePrefix);
		Prefixes.Add(Settings->MaterialPrefix);
		Prefixes.Add(Settings->MaterialInstancePrefix);

		for (const FSmartAssetPrefixRule& Rule : Settings->PrefixRules)
		{
			if (Rule.bEnabled && !Rule.Prefix.IsEmpty())
			{
				Prefixes.Add(Rule.Prefix);
			}
		}
	}

	Prefixes.Sort([](const FString& A, const FString& B)
	{
		return A.Len() > B.Len();
	});

	for (const FString& Prefix : Prefixes)
	{
		if (!Prefix.IsEmpty() && InName.StartsWith(Prefix))
		{
			return InName.RightChop(Prefix.Len());
		}
	}

	return InName;
}

FString FSmartAssetRuleResolver::ResolveBuiltInPrefix(const FSmartAssetCreateRequest& Request)
{
	const USmartAssetSettings* Settings = GetSettings();
	check(Settings);

	switch (Request.UnderlyingKind)
	{
	case ESmartUnderlyingAssetKind::DataTable:
		return Settings->DataTablePrefix;
	case ESmartUnderlyingAssetKind::Material:
		return Settings->MaterialPrefix;
	case ESmartUnderlyingAssetKind::MaterialInstance:
		return Settings->MaterialInstancePrefix;
	default:
		return TEXT("AS_");
	}
}

FString FSmartAssetRuleResolver::BuildNameStem(const FSmartAssetCreateRequest& Request)
{
	const USmartAssetSettings* Settings = GetSettings();

	if (Request.ParentBlueprint && Settings && Settings->bUseParentBlueprintAssetNameForChildren)
	{
		const FString RawName = Request.ParentBlueprint->GetName();
		return Settings->bStripKnownPrefixFromParentBlueprintName ? StripKnownPrefix(RawName) : RawName;
	}

	switch (Request.UnderlyingKind)
	{
	case ESmartUnderlyingAssetKind::Blueprint:
	case ESmartUnderlyingAssetKind::DataAsset:
		return StripKnownPrefix(GetClassStem(Request.ParentClass));
	case ESmartUnderlyingAssetKind::DataTable:
		return StripKnownPrefix(GetStructStem(Request.RowStruct));
	case ESmartUnderlyingAssetKind::Material:
		return TEXT("Material");
	case ESmartUnderlyingAssetKind::MaterialInstance:
		if (Request.ParentMaterial)
		{
			return StripKnownPrefix(Request.ParentMaterial->GetName());
		}
		return TEXT("MaterialInstance");
	default:
		return TEXT("Asset");
	}
}
