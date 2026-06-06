#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Engine/DataAsset.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Rule/SmartAssetRuleResolver.h"
#include "Service/SmartAssetCreationService.h"
#include "Service/SmartCreationOptionBuilder.h"
#include "Settings/SmartAssetSettings.h"
#include "UObject/UnrealType.h"

namespace
{
	FSmartAssetPrefixRule MakeRule(UClass* BaseClass, const FString& Prefix, bool bIncludeDerivedClasses = true)
	{
		FSmartAssetPrefixRule Rule;
		Rule.BaseClass = BaseClass;
		Rule.Prefix = Prefix;
		Rule.bIncludeDerivedClasses = bIncludeDerivedClasses;
		return Rule;
	}

	class FScopedPrefixRules
	{
	public:
		explicit FScopedPrefixRules(USmartAssetSettings* InSettings)
			: Settings(InSettings)
			, OriginalRules(InSettings->PrefixRules)
		{
		}

		~FScopedPrefixRules()
		{
			Settings->PrefixRules = OriginalRules;
		}

	private:
		USmartAssetSettings* Settings;
		TArray<FSmartAssetPrefixRule> OriginalRules;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmartAssetCreatorRuleResolutionTest,
	"SmartAssetCreator.Rules.Resolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSmartAssetCreatorRuleResolutionTest::RunTest(const FString& Parameters)
{
	USmartAssetSettings* Settings = GetMutableDefault<USmartAssetSettings>();
	FScopedPrefixRules RestoreRules(Settings);

	Settings->PrefixRules = {
		MakeRule(UObject::StaticClass(), TEXT("ROOT_")),
		MakeRule(UUserWidget::StaticClass(), TEXT("WIDGET_"))
	};

	FSmartAssetCreateRequest Request;
	Request.UnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;
	Request.ParentClass = UUserWidget::StaticClass();
	TestEqual(TEXT("Closest matching rule wins"), FSmartAssetRuleResolver::ResolvePrefix(Request), FString(TEXT("WIDGET_")));

	Request.bHasPrefixFallback = true;
	Request.PrefixFallback = TEXT("WBP_");
	Settings->PrefixRules = { MakeRule(UObject::StaticClass(), TEXT("EXACT_"), false) };
	TestEqual(TEXT("Template fallback is used when no class rule matches"), FSmartAssetRuleResolver::ResolvePrefix(Request), FString(TEXT("WBP_")));

	Request.ParentClass = UObject::StaticClass();
	TestEqual(TEXT("Exact-only rule matches its class"), FSmartAssetRuleResolver::ResolvePrefix(Request), FString(TEXT("EXACT_")));

	Request.bHasPrefixOverride = true;
	Request.PrefixOverride.Reset();
	TestEqual(TEXT("Explicit empty prefix remains empty"), FSmartAssetRuleResolver::ResolvePrefix(Request), FString());
	Request.bHasPrefixOverride = false;
	Request.bHasPrefixFallback = false;

	Settings->PrefixRules.Reset();
	Request.ParentClass = UUserWidget::StaticClass();
	TestEqual(TEXT("Class-based requests without a template fallback use the generic asset prefix"), FSmartAssetRuleResolver::ResolvePrefix(Request), FString(TEXT("AS_")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmartAssetCreatorOptionBuilderTest,
	"SmartAssetCreator.Rules.MultiplePrefixes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSmartAssetCreatorOptionBuilderTest::RunTest(const FString& Parameters)
{
	USmartAssetSettings* Settings = GetMutableDefault<USmartAssetSettings>();
	FScopedPrefixRules RestoreRules(Settings);
	bool bSettingsChangedBroadcast = false;
	const FDelegateHandle SettingsChangedHandle = USmartAssetSettings::OnSettingsChanged().AddLambda([&bSettingsChangedBroadcast]()
	{
		bSettingsChangedBroadcast = true;
	});
	FPropertyChangedEvent PropertyChangedEvent(nullptr);
	Settings->PostEditChangeProperty(PropertyChangedEvent);
	USmartAssetSettings::OnSettingsChanged().Remove(SettingsChangedHandle);
	TestTrue(TEXT("Editing Smart Asset Creator settings broadcasts a refresh notification"), bSettingsChangedBroadcast);

	Settings->PrefixRules = {
		MakeRule(AActor::StaticClass(), TEXT("BP_")),
		MakeRule(AActor::StaticClass(), TEXT("ALT_"))
	};

	FSmartCreationOptionBuilder Builder;
	const TArray<FSmartCreationOption> Options = Builder.BuildOptions();
	int32 UserDefinedObjectOptions = 0;
	for (const FSmartCreationOption& Option : Options)
	{
		if (Option.bIsUserDefined && Option.ParentClass == AActor::StaticClass())
		{
			++UserDefinedObjectOptions;
		}
	}

	TestEqual(TEXT("Duplicate class rules remain available as separate prefix options"), UserDefinedObjectOptions, 2);

	Settings->PrefixRules = {
		MakeRule(UObject::StaticClass(), TEXT("BP_")),
		MakeRule(UInterface::StaticClass(), TEXT("BPI_")),
		MakeRule(UDataAsset::StaticClass(), TEXT("DA_"))
	};
	const TArray<FSmartCreationOption> DefaultEquivalentOptions = Builder.BuildOptions();
	TestEqual(TEXT("Rules equivalent to built-in templates are not duplicated"), DefaultEquivalentOptions.Num(), 8);
	TestTrue(TEXT("An equivalent user rule keeps the built-in template dynamic"), DefaultEquivalentOptions.ContainsByPredicate([](const FSmartCreationOption& Option)
	{
		return Option.ParentClass == UObject::StaticClass() && !Option.bIsUserDefined;
	}));
	TestTrue(TEXT("Every creation option displays its prefix"), DefaultEquivalentOptions.ContainsByPredicate([](const FSmartCreationOption& Option)
	{
		return Option.ParentClass == UObject::StaticClass() && Option.DisplayName.ToString() == TEXT("Object Blueprint [BP_]");
	}));
	TestTrue(TEXT("Widget Blueprint template keeps its explicit creation kind"), DefaultEquivalentOptions.ContainsByPredicate([](const FSmartCreationOption& Option)
	{
		return Option.ParentClass == UUserWidget::StaticClass()
			&& Option.BlueprintTemplateKind == ESmartBlueprintTemplateKind::Widget
			&& !Option.bIsUserDefined;
	}));

	Settings->PrefixRules = {
		MakeRule(UObject::StaticClass(), TEXT("ALT_"))
	};
	const TArray<FSmartCreationOption> AlternatePrefixOptions = Builder.BuildOptions();
	TestEqual(TEXT("A root-class rule customizes the built-in template without duplicating it"), AlternatePrefixOptions.Num(), 8);
	const FSmartCreationOption* CustomizedObjectOption = AlternatePrefixOptions.FindByPredicate([](const FSmartCreationOption& Option)
	{
		return Option.ParentClass == UObject::StaticClass();
	});
	TestTrue(TEXT("The built-in Object template uses the configured prefix"), CustomizedObjectOption && CustomizedObjectOption->Prefix == TEXT("ALT_"));
	TestTrue(TEXT("The configured prefix is shown in the option label"), CustomizedObjectOption && CustomizedObjectOption->DisplayName.ToString() == TEXT("Object Blueprint [ALT_]"));
	if (CustomizedObjectOption)
	{
		Settings->PrefixRules.Add(MakeRule(AActor::StaticClass(), TEXT("ACT_")));
		FSmartAssetCreateRequest DerivedClassRequest;
		DerivedClassRequest.UnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;
		DerivedClassRequest.ParentClass = AActor::StaticClass();
		DerivedClassRequest.bHasPrefixFallback = true;
		DerivedClassRequest.PrefixFallback = CustomizedObjectOption->Prefix;
		TestEqual(
			TEXT("A built-in template displays the prefix that creation will actually use"),
			FSmartCreationOptionBuilder::MakeEffectiveDisplayName(*CustomizedObjectOption, DerivedClassRequest).ToString(),
			FString(TEXT("Object Blueprint [ACT_]")));
	}

	FSmartCreationOption DisambiguatedOption;
	DisambiguatedOption.TemplateName = FText::FromString(TEXT("Object Blueprint (UObject 1)"));
	FSmartAssetCreateRequest DisambiguatedRequest;
	DisambiguatedRequest.bHasPrefixOverride = true;
	DisambiguatedRequest.PrefixOverride = TEXT("ACT_");
	TestEqual(
		TEXT("Dynamic prefix display preserves duplicate-option disambiguation"),
		FSmartCreationOptionBuilder::MakeEffectiveDisplayName(DisambiguatedOption, DisambiguatedRequest).ToString(),
		FString(TEXT("Object Blueprint (UObject 1) [ACT_]")));

	Settings->PrefixRules = {
		MakeRule(AActor::StaticClass(), FString())
	};
	const TArray<FSmartCreationOption> EmptyPrefixOptions = Builder.BuildOptions();
	TestTrue(TEXT("An empty prefix is shown explicitly"), EmptyPrefixOptions.ContainsByPredicate([](const FSmartCreationOption& Option)
	{
		return Option.ParentClass == AActor::StaticClass() && Option.DisplayName.ToString().Contains(TEXT("[No Prefix]"));
	}));

	Settings->PrefixRules = { MakeRule(UObject::StaticClass(), TEXT("ABSTRACT_BASE_"), true) };
	const TArray<FSmartCreationOption> AbstractBaseOptions = Builder.BuildOptions();
	const FSmartCreationOption* AbstractBaseOption = AbstractBaseOptions.FindByPredicate([](const FSmartCreationOption& Option)
	{
		return Option.ParentClass == UObject::StaticClass() && Option.Prefix == TEXT("ABSTRACT_BASE_");
	});
	TestNotNull(TEXT("Abstract base class remains available when derived classes are included"), AbstractBaseOption);
	if (AbstractBaseOption)
	{
		TestTrue(TEXT("Derived class matches an include-derived abstract template"), FSmartCreationOptionBuilder::DoesOptionMatchClass(*AbstractBaseOption, AActor::StaticClass()));
	}

	FSmartCreationOption ExactOption;
	ExactOption.ParentClass = AActor::StaticClass();
	ExactOption.bAllowDerivedClasses = false;
	TestFalse(TEXT("Exact-only option rejects derived classes"), FSmartCreationOptionBuilder::DoesOptionMatchClass(ExactOption, APawn::StaticClass()));
	TestTrue(TEXT("Exact-only option accepts its exact class"), FSmartCreationOptionBuilder::DoesOptionMatchClass(ExactOption, AActor::StaticClass()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmartAssetCreatorRequestValidationTest,
	"SmartAssetCreator.Creation.RequestValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSmartAssetCreatorRequestValidationTest::RunTest(const FString& Parameters)
{
	FSmartAssetCreationService Service;
	FSmartAssetCreateRequest Request;
	Request.TargetFolder = TEXT("/Engine/ShouldNotWrite");

	const FSmartAssetCreateResult Result = Service.CreateAsset(Request);
	TestFalse(TEXT("Engine Content creation is rejected"), Result.bSucceeded);
	TestTrue(TEXT("Rejected request includes a useful error"), !Result.ErrorMessage.IsEmpty());

	Request.TargetFolder = TEXT("/Game");
	Request.UnderlyingKind = ESmartUnderlyingAssetKind::DataAsset;
	Request.ParentClass = UDataAsset::StaticClass();
	TestTrue(TEXT("Abstract Data Asset class is rejected"), !Service.ValidateRequest(Request).IsEmpty());

	Request.UnderlyingKind = ESmartUnderlyingAssetKind::DataTable;
	Request.ParentClass = nullptr;
	Request.RowStruct = nullptr;
	TestTrue(TEXT("Data Table without row struct is rejected"), !Service.ValidateRequest(Request).IsEmpty());
	Request.RowStruct = TBaseStructure<FVector>::Get();
	TestTrue(TEXT("Data Table with an invalid row struct is rejected"), !Service.ValidateRequest(Request).IsEmpty());

	Request.UnderlyingKind = ESmartUnderlyingAssetKind::MaterialInstance;
	Request.RowStruct = nullptr;
	Request.ParentMaterial = nullptr;
	TestTrue(TEXT("Material Instance without parent material is rejected"), !Service.ValidateRequest(Request).IsEmpty());

	Request.UnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;
	Request.ParentClass = UObject::StaticClass();
	TestTrue(TEXT("Default Object Blueprint request remains valid"), Service.ValidateRequest(Request).IsEmpty());

	Request.ParentClass = nullptr;
	Request.BlueprintTemplateKind = ESmartBlueprintTemplateKind::Widget;
	TestTrue(TEXT("Widget Blueprint request defaults to a Widget parent class"), Service.ValidateRequest(Request).IsEmpty());
	Request.ParentClass = UObject::StaticClass();
	TestTrue(TEXT("Widget Blueprint rejects non-Widget parent classes"), !Service.ValidateRequest(Request).IsEmpty());
	Request.BlueprintTemplateKind = ESmartBlueprintTemplateKind::Normal;

	UBlueprint* UncompiledParent = NewObject<UBlueprint>();
	Request.ParentBlueprint = UncompiledParent;
	Request.ParentClass = nullptr;
	TestTrue(TEXT("Child creation rejects a parent without a generated class"), !Service.ValidateRequest(Request).IsEmpty());

	UBlueprint* InterfaceParent = NewObject<UBlueprint>();
	InterfaceParent->BlueprintType = BPTYPE_Interface;
	InterfaceParent->GeneratedClass = UInterface::StaticClass();
	Request.ParentBlueprint = InterfaceParent;
	TestTrue(TEXT("Child creation rejects Interface Blueprints"), !Service.ValidateRequest(Request).IsEmpty());

	UBlueprint* FunctionLibraryParent = NewObject<UBlueprint>();
	FunctionLibraryParent->BlueprintType = BPTYPE_FunctionLibrary;
	FunctionLibraryParent->GeneratedClass = UBlueprintFunctionLibrary::StaticClass();
	Request.ParentBlueprint = FunctionLibraryParent;
	TestTrue(TEXT("Child creation rejects Blueprint Function Libraries"), !Service.ValidateRequest(Request).IsEmpty());

	Request.ParentBlueprint = nullptr;
	Request.ParentClass = UAnimInstance::StaticClass();
	Request.BlueprintTemplateKind = ESmartBlueprintTemplateKind::Anim;
	Request.TargetSkeleton = nullptr;
	Request.bTemplateAnimBlueprint = false;
	TestTrue(TEXT("Non-template Anim Blueprint requires a Skeleton"), !Service.ValidateRequest(Request).IsEmpty());
	Request.bTemplateAnimBlueprint = true;
	TestTrue(TEXT("Template Anim Blueprint can omit a Skeleton"), Service.ValidateRequest(Request).IsEmpty());

	UAnimBlueprint* ParentAnimBlueprint = NewObject<UAnimBlueprint>();
	ParentAnimBlueprint->GeneratedClass = UAnimInstance::StaticClass();
	ParentAnimBlueprint->TargetSkeleton = NewObject<USkeleton>();
	Request.ParentBlueprint = ParentAnimBlueprint;
	Request.ParentClass = nullptr;
	Request.bTemplateAnimBlueprint = false;
	TestTrue(TEXT("Child Anim Blueprint inherits its parent's Skeleton"), Service.ValidateRequest(Request).IsEmpty());

	Request.ParentBlueprint = nullptr;
	Request.ParentClass = UInterface::StaticClass();
	Request.BlueprintTemplateKind = ESmartBlueprintTemplateKind::Interface;
	TestTrue(TEXT("Interface Blueprint request remains valid without a selectable parent"), Service.ValidateRequest(Request).IsEmpty());
	return true;
}

#endif
