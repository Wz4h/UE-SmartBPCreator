#include "Service/SmartCreationOptionBuilder.h"

#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Rule/SmartAssetPrefixRule.h"
#include "Rule/SmartAssetRuleResolver.h"
#include "Settings/SmartAssetSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogSmartCreationOptionBuilder, Log, All);

namespace
{
	bool AreEquivalentOptions(const FSmartCreationOption& A, const FSmartCreationOption& B)
	{
		return A.UnderlyingKind == B.UnderlyingKind
			&& A.BlueprintTemplateKind == B.BlueprintTemplateKind
			&& A.ParentClass == B.ParentClass
			&& A.Prefix == B.Prefix
			&& A.bAllowDerivedClasses == B.bAllowDerivedClasses;
	}

	FSmartCreationOption MakeClassOption(
		const FText& DisplayName,
		ESmartUnderlyingAssetKind UnderlyingKind,
		ESmartBlueprintTemplateKind BlueprintTemplateKind,
		UClass* ParentClass,
		const FString& Prefix,
		bool bIsUserDefined,
		bool bAllowDerivedClasses = true)
	{
		FSmartCreationOption Option;
		Option.DisplayName = DisplayName;
		Option.TemplateName = DisplayName;
		Option.UnderlyingKind = UnderlyingKind;
		Option.BlueprintTemplateKind = BlueprintTemplateKind;
		Option.ParentClass = ParentClass;
		Option.Prefix = Prefix;
		Option.bIsUserDefined = bIsUserDefined;
		Option.bAllowDerivedClasses = bAllowDerivedClasses;
		return Option;
	}

	FSmartCreationOption MakeAssetOption(
		const FText& DisplayName,
		ESmartUnderlyingAssetKind UnderlyingKind,
		const FString& Prefix)
	{
		FSmartCreationOption Option;
		Option.DisplayName = DisplayName;
		Option.TemplateName = DisplayName;
		Option.UnderlyingKind = UnderlyingKind;
		Option.BlueprintTemplateKind = ESmartBlueprintTemplateKind::Normal;
		Option.Prefix = Prefix;
		return Option;
	}

}

TArray<FSmartCreationOption> FSmartCreationOptionBuilder::BuildOptions()
{
	TArray<FSmartCreationOption> Options;
	AddBuiltInOptions(Options);
	AddUserDefinedOptions(Options);
	for (FSmartCreationOption& Option : Options)
	{
		Option.DisplayName = MakeDisplayName(Option, Option.Prefix);
	}
	DisambiguateDisplayNames(Options);
	return Options;
}

void FSmartCreationOptionBuilder::ResetCache()
{
	ClassCache.Reset();
	FailedClassPaths.Reset();
}

ESmartBlueprintTemplateKind FSmartCreationOptionBuilder::ResolveBlueprintTemplateKind(const UClass* ParentClass)
{
	if (!ParentClass)
	{
		return ESmartBlueprintTemplateKind::Normal;
	}

	if (ParentClass->IsChildOf(UUserWidget::StaticClass()))
	{
		return ESmartBlueprintTemplateKind::Widget;
	}

	if (ParentClass->IsChildOf(UAnimInstance::StaticClass()))
	{
		return ESmartBlueprintTemplateKind::Anim;
	}

	if (ParentClass->IsChildOf(UInterface::StaticClass()))
	{
		return ESmartBlueprintTemplateKind::Interface;
	}

	return ESmartBlueprintTemplateKind::Normal;
}

bool FSmartCreationOptionBuilder::DoesOptionMatchClass(const FSmartCreationOption& Option, const UClass* CandidateClass)
{
	if (!Option.ParentClass || !CandidateClass)
	{
		return false;
	}

	return Option.bAllowDerivedClasses
		? CandidateClass->IsChildOf(Option.ParentClass)
		: CandidateClass == Option.ParentClass;
}

FText FSmartCreationOptionBuilder::MakeDisplayName(const FSmartCreationOption& Option, const FString& Prefix)
{
	const FText PrefixText = Prefix.IsEmpty()
		? NSLOCTEXT("SmartAssetCreator", "NoPrefixLabel", "No Prefix")
		: FText::FromString(Prefix);
	return FText::Format(
		NSLOCTEXT("SmartAssetCreator", "CreationOptionWithPrefix", "{0} [{1}]"),
		Option.TemplateName,
		PrefixText);
}

FText FSmartCreationOptionBuilder::MakeEffectiveDisplayName(const FSmartCreationOption& Option, const FSmartAssetCreateRequest& Request)
{
	return MakeDisplayName(Option, FSmartAssetRuleResolver::ResolvePrefix(Request));
}

void FSmartCreationOptionBuilder::AddBuiltInOptions(TArray<FSmartCreationOption>& OutOptions) const
{
	const USmartAssetSettings* Settings = GetDefault<USmartAssetSettings>();
	const FString DataTablePrefix = Settings ? Settings->DataTablePrefix : TEXT("DT_");
	const FString MaterialPrefix = Settings ? Settings->MaterialPrefix : TEXT("M_");
	const FString MaterialInstancePrefix = Settings ? Settings->MaterialInstancePrefix : TEXT("MI_");

	OutOptions.Add(MakeClassOption(
		NSLOCTEXT("SmartAssetCreator", "ObjectBlueprintCreationOption", "Object Blueprint"),
		ESmartUnderlyingAssetKind::Blueprint,
		ESmartBlueprintTemplateKind::Normal,
		UObject::StaticClass(),
		FSmartAssetRuleResolver::ResolveClassPrefix(UObject::StaticClass(), TEXT("BP_")),
		false));

	OutOptions.Add(MakeClassOption(
		NSLOCTEXT("SmartAssetCreator", "WidgetBlueprintCreationOption", "Widget Blueprint"),
		ESmartUnderlyingAssetKind::Blueprint,
		ESmartBlueprintTemplateKind::Widget,
		UUserWidget::StaticClass(),
		FSmartAssetRuleResolver::ResolveClassPrefix(UUserWidget::StaticClass(), TEXT("WBP_")),
		false));

	OutOptions.Add(MakeClassOption(
		NSLOCTEXT("SmartAssetCreator", "AnimBlueprintCreationOption", "Anim Blueprint"),
		ESmartUnderlyingAssetKind::Blueprint,
		ESmartBlueprintTemplateKind::Anim,
		UAnimInstance::StaticClass(),
		FSmartAssetRuleResolver::ResolveClassPrefix(UAnimInstance::StaticClass(), TEXT("ABP_")),
		false));

	OutOptions.Add(MakeClassOption(
		NSLOCTEXT("SmartAssetCreator", "InterfaceBlueprintCreationOption", "Interface Blueprint"),
		ESmartUnderlyingAssetKind::Blueprint,
		ESmartBlueprintTemplateKind::Interface,
		UInterface::StaticClass(),
		FSmartAssetRuleResolver::ResolveClassPrefix(UInterface::StaticClass(), TEXT("BPI_")),
		false));

	OutOptions.Add(MakeClassOption(
		NSLOCTEXT("SmartAssetCreator", "DataAssetCreationOption", "Data Asset"),
		ESmartUnderlyingAssetKind::DataAsset,
		ESmartBlueprintTemplateKind::Normal,
		UDataAsset::StaticClass(),
		FSmartAssetRuleResolver::ResolveClassPrefix(UDataAsset::StaticClass(), TEXT("DA_")),
		false));

	OutOptions.Add(MakeAssetOption(
		NSLOCTEXT("SmartAssetCreator", "DataTableCreationOption", "Data Table"),
		ESmartUnderlyingAssetKind::DataTable,
		DataTablePrefix));

	OutOptions.Add(MakeAssetOption(
		NSLOCTEXT("SmartAssetCreator", "MaterialCreationOption", "Material"),
		ESmartUnderlyingAssetKind::Material,
		MaterialPrefix));

	OutOptions.Add(MakeAssetOption(
		NSLOCTEXT("SmartAssetCreator", "MaterialInstanceCreationOption", "Material Instance"),
		ESmartUnderlyingAssetKind::MaterialInstance,
		MaterialInstancePrefix));
}

void FSmartCreationOptionBuilder::AddUserDefinedOptions(TArray<FSmartCreationOption>& OutOptions)
{
	const USmartAssetSettings* Settings = GetDefault<USmartAssetSettings>();
	if (!Settings)
	{
		return;
	}

	for (const FSmartAssetPrefixRule& Rule : Settings->PrefixRules)
	{
		FSmartCreationOption Option;
		if (!TryBuildRuleOption(Rule, Option))
		{
			continue;
		}

		if (OutOptions.ContainsByPredicate([&Option](const FSmartCreationOption& ExistingOption)
		{
			return AreEquivalentOptions(ExistingOption, Option);
		}))
		{
			// A rule identical to a built-in template is redundant. The built-in
			// option remains dynamic so a more specific selected class can win.
			continue;
		}

		OutOptions.Add(Option);
	}
}

UClass* FSmartCreationOptionBuilder::ResolveRuleClass(const FSoftClassPath& ClassPath)
{
	const FString ClassPathString = ClassPath.ToString();
	if (ClassPathString.IsEmpty())
	{
		return nullptr;
	}

	if (FailedClassPaths.Contains(ClassPathString))
	{
		return nullptr;
	}

	if (const TWeakObjectPtr<UClass>* CachedClass = ClassCache.Find(ClassPathString))
	{
		if (CachedClass->IsValid())
		{
			return CachedClass->Get();
		}
	}

	UClass* LoadedClass = ClassPath.TryLoadClass<UObject>();
	if (LoadedClass)
	{
		ClassCache.Add(ClassPathString, LoadedClass);
	}
	else
	{
		FailedClassPaths.Add(ClassPathString);
	}
	return LoadedClass;
}

bool FSmartCreationOptionBuilder::TryBuildRuleOption(const FSmartAssetPrefixRule& Rule, FSmartCreationOption& OutOption)
{
	if (!Rule.bEnabled)
	{
		return false;
	}

	if (Rule.BaseClass.ToString().IsEmpty())
	{
		UE_LOG(LogSmartCreationOptionBuilder, Warning, TEXT("Skipped class prefix rule because its class path is empty."));
		return false;
	}

	UClass* RuleClass = ResolveRuleClass(Rule.BaseClass);
	if (!RuleClass)
	{
		UE_LOG(LogSmartCreationOptionBuilder, Warning, TEXT("Skipped class prefix rule because '%s' could not be loaded."), *Rule.BaseClass.ToString());
		return false;
	}

	if (RuleClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		UE_LOG(LogSmartCreationOptionBuilder, Warning, TEXT("Skipped class prefix rule for '%s' because the class is deprecated or superseded."), *RuleClass->GetName());
		return false;
	}

	if (RuleClass->HasAnyClassFlags(CLASS_Abstract) && !Rule.bIncludeDerivedClasses)
	{
		UE_LOG(LogSmartCreationOptionBuilder, Warning, TEXT("Skipped exact-only class prefix rule for abstract class '%s' because it cannot be created directly."), *RuleClass->GetName());
		return false;
	}

	if (RuleClass->IsChildOf(UDataAsset::StaticClass()))
	{
			OutOption = MakeClassOption(
				MakeClassDisplayName(RuleClass),
				ESmartUnderlyingAssetKind::DataAsset,
				ESmartBlueprintTemplateKind::Normal,
				RuleClass,
				Rule.Prefix,
				true,
				Rule.bIncludeDerivedClasses);
		return true;
	}

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(RuleClass))
	{
		UE_LOG(LogSmartCreationOptionBuilder, Warning, TEXT("Skipped class prefix rule for '%s' because it cannot be used as a Blueprint parent class."), *RuleClass->GetName());
		return false;
	}

		OutOption = MakeClassOption(
			MakeClassDisplayName(RuleClass),
			ESmartUnderlyingAssetKind::Blueprint,
			ResolveBlueprintTemplateKind(RuleClass),
			RuleClass,
			Rule.Prefix,
			true,
			Rule.bIncludeDerivedClasses);
	return true;
}

FText FSmartCreationOptionBuilder::MakeClassDisplayName(const UClass* InClass) const
{
	if (!InClass)
	{
		return FText::GetEmpty();
	}

	FString ClassName = InClass->GetName();
	if (ClassName.EndsWith(TEXT("_C")))
	{
		ClassName.LeftChopInline(2, EAllowShrinking::No);
	}
	if (ClassName.StartsWith(TEXT("U")) || ClassName.StartsWith(TEXT("A")))
	{
		ClassName.RightChopInline(1, EAllowShrinking::No);
	}

	const FString Suffix = InClass->IsChildOf(UDataAsset::StaticClass()) ? TEXT(" Data Asset") : TEXT(" Blueprint");
	return FText::FromString(FName::NameToDisplayString(ClassName, false) + Suffix);
}

void FSmartCreationOptionBuilder::DisambiguateDisplayNames(TArray<FSmartCreationOption>& Options) const
{
	TMap<FString, int32> Counts;
	for (const FSmartCreationOption& Option : Options)
	{
		Counts.FindOrAdd(Option.DisplayName.ToString())++;
	}

	TMap<FString, int32> Seen;
	for (FSmartCreationOption& Option : Options)
	{
		const FString DisplayName = Option.DisplayName.ToString();
		if (Counts.FindRef(DisplayName) <= 1)
		{
			continue;
		}

		const int32 Index = ++Seen.FindOrAdd(DisplayName);
		const FString ClassSuffix = Option.ParentClass ? Option.ParentClass->GetName() : TEXT("Asset");
		Option.TemplateName = FText::FromString(FString::Printf(
			TEXT("%s (%s %d)"),
			*Option.TemplateName.ToString(),
			*ClassSuffix,
			Index));
		Option.DisplayName = MakeDisplayName(Option, Option.Prefix);
		UE_LOG(LogSmartCreationOptionBuilder, Warning, TEXT("Creation option display name '%s' is duplicated; renamed one option for clarity."), *DisplayName);
	}
}
