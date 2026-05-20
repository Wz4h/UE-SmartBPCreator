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

FString FSmartAssetRuleResolver::ResolvePrefix(const FSmartAssetCreateRequest& Request)
{
	const USmartAssetSettings* Settings = GetSettings();
	UClass* RuleCandidateClass = ResolveRuleClassCandidate(Request);
	const bool bUsesClassRules = UsesClassRules(Request.AssetType);

	if (bUsesClassRules && RuleCandidateClass && Settings)
	{
		int32 BestDistance = TNumericLimits<int32>::Max();
		const FSmartAssetPrefixRule* BestRule = nullptr;

		for (const FSmartAssetPrefixRule& Rule : Settings->PrefixRules)
		{
			if (!Rule.bEnabled || Rule.Prefix.IsEmpty())
			{
				continue;
			}

			UClass* RuleClass = Rule.BaseClass.TryLoadClass<UObject>();
			if (!RuleClass)
			{
				continue;
			}

			int32 Distance = INDEX_NONE;
			if (Rule.bIncludeDerivedClasses)
			{
				Distance = ComputeInheritanceDistance(RuleCandidateClass, RuleClass);
			}
			else if (RuleCandidateClass == RuleClass)
			{
				Distance = 0;
			}

			if (Distance != INDEX_NONE && Distance < BestDistance)
			{
				BestDistance = Distance;
				BestRule = &Rule;
			}
		}

		if (BestRule)
		{
			return BestRule->Prefix;
		}
	}

	return bUsesClassRules ? FString() : ResolveBuiltInPrefix(Request);
}

FString FSmartAssetRuleResolver::BuildAssetBaseName(const FSmartAssetCreateRequest& Request)
{
	const USmartAssetSettings* Settings = GetSettings();
	const FString Prefix = ResolvePrefix(Request);
	const FString Stem = BuildNameStem(Request);

	const bool bIsChildBlueprint = Request.ParentBlueprint != nullptr
		&& (Request.AssetType == ESmartAssetType::ActorBlueprint
			|| Request.AssetType == ESmartAssetType::WidgetBlueprint
			|| Request.AssetType == ESmartAssetType::AnimBlueprint
			|| Request.AssetType == ESmartAssetType::InterfaceBlueprint);

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
			if (!Rule.Prefix.IsEmpty())
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

	switch (Request.AssetType)
	{
	case ESmartAssetType::DataTable:
		return Settings->DataTablePrefix;
	case ESmartAssetType::Material:
		return Settings->MaterialPrefix;
	case ESmartAssetType::MaterialInstance:
		return Settings->MaterialInstancePrefix;
	default:
		return TEXT("AS_");
	}
}

bool FSmartAssetRuleResolver::UsesClassRules(ESmartAssetType AssetType)
{
	switch (AssetType)
	{
	case ESmartAssetType::ActorBlueprint:
	case ESmartAssetType::WidgetBlueprint:
	case ESmartAssetType::AnimBlueprint:
	case ESmartAssetType::InterfaceBlueprint:
	case ESmartAssetType::DataAsset:
		return true;
	default:
		return false;
	}
}

UClass* FSmartAssetRuleResolver::ResolveRuleClassCandidate(const FSmartAssetCreateRequest& Request)
{
	if (Request.ParentBlueprint && Request.ParentBlueprint->GeneratedClass)
	{
		return Request.ParentBlueprint->GeneratedClass;
	}

	if (Request.ParentClass)
	{
		return Request.ParentClass;
	}

	switch (Request.AssetType)
	{
	case ESmartAssetType::ActorBlueprint:
		return UObject::StaticClass();
	case ESmartAssetType::WidgetBlueprint:
		return UUserWidget::StaticClass();
	case ESmartAssetType::AnimBlueprint:
		return UAnimInstance::StaticClass();
	case ESmartAssetType::InterfaceBlueprint:
		return UInterface::StaticClass();
	case ESmartAssetType::DataAsset:
		return UDataAsset::StaticClass();
	case ESmartAssetType::DataTable:
		return UDataTable::StaticClass();
	case ESmartAssetType::Material:
		return UMaterial::StaticClass();
	case ESmartAssetType::MaterialInstance:
		return UMaterialInstanceConstant::StaticClass();
	default:
		return nullptr;
	}
}

int32 FSmartAssetRuleResolver::ComputeInheritanceDistance(const UClass* ChildClass, const UClass* AncestorClass)
{
	if (!ChildClass || !AncestorClass)
	{
		return INDEX_NONE;
	}

	int32 Distance = 0;
	for (const UClass* Current = ChildClass; Current; Current = Current->GetSuperClass(), ++Distance)
	{
		if (Current == AncestorClass)
		{
			return Distance;
		}
	}

	return INDEX_NONE;
}

FString FSmartAssetRuleResolver::BuildNameStem(const FSmartAssetCreateRequest& Request)
{
	const USmartAssetSettings* Settings = GetSettings();

	if (Request.ParentBlueprint && Settings && Settings->bUseParentBlueprintAssetNameForChildren)
	{
		const FString RawName = Request.ParentBlueprint->GetName();
		return Settings->bStripKnownPrefixFromParentBlueprintName ? StripKnownPrefix(RawName) : RawName;
	}

	switch (Request.AssetType)
	{
	case ESmartAssetType::ActorBlueprint:
	case ESmartAssetType::WidgetBlueprint:
	case ESmartAssetType::AnimBlueprint:
	case ESmartAssetType::InterfaceBlueprint:
	case ESmartAssetType::DataAsset:
		return StripKnownPrefix(GetClassStem(Request.ParentClass));
	case ESmartAssetType::DataTable:
		return StripKnownPrefix(GetStructStem(Request.RowStruct));
	case ESmartAssetType::Material:
		return TEXT("Material");
	case ESmartAssetType::MaterialInstance:
		if (Request.ParentMaterial)
		{
			return StripKnownPrefix(Request.ParentMaterial->GetName());
		}
		return TEXT("MaterialInstance");
	default:
		return TEXT("Asset");
	}
}
