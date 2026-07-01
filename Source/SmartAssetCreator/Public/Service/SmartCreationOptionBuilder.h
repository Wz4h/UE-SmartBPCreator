// Copyright (c) 2026 Wz4h. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Core/SmartCreationOption.h"
#include "Rule/SmartAssetPrefixRule.h"

struct FSmartAssetCreateRequest;

class SMARTASSETCREATOR_API FSmartCreationOptionBuilder
{
public:
	TArray<FSmartCreationOption> BuildOptions();
	void ResetCache();

	static ESmartBlueprintTemplateKind ResolveBlueprintTemplateKind(const UClass* ParentClass);
	static bool DoesOptionMatchClass(const FSmartCreationOption& Option, const UClass* CandidateClass);
	static FText MakeDisplayName(const FSmartCreationOption& Option, const FString& Prefix);
	static FText MakeEffectiveDisplayName(const FSmartCreationOption& Option, const FSmartAssetCreateRequest& Request);

private:
	void AddBuiltInOptions(TArray<FSmartCreationOption>& OutOptions) const;
	void AddUserDefinedOptions(TArray<FSmartCreationOption>& OutOptions);
	UClass* ResolveRuleClass(const FSoftClassPath& ClassPath);
	bool TryBuildRuleOption(const FSmartAssetPrefixRule& Rule, FSmartCreationOption& OutOption);
	FText MakeClassDisplayName(const UClass* InClass) const;
	void DisambiguateDisplayNames(TArray<FSmartCreationOption>& Options) const;

private:
	TMap<FString, TWeakObjectPtr<UClass>> ClassCache;
	TSet<FString> FailedClassPaths;
};
