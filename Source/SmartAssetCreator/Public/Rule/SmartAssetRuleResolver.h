#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"

class USmartAssetSettings;

class SMARTASSETCREATOR_API FSmartAssetRuleResolver
{
public:
	static const USmartAssetSettings* GetSettings();
	static void LoadConfiguredRuleClasses();
	static FString ResolvePrefix(const FSmartAssetCreateRequest& Request);
	static FString ResolveClassPrefix(const UClass* ParentClass, const FString& FallbackPrefix);
	static FString BuildAssetBaseName(const FSmartAssetCreateRequest& Request);
	static FString StripKnownPrefix(const FString& InName);

private:
	static FString ResolveBuiltInPrefix(const FSmartAssetCreateRequest& Request);
	static FString BuildNameStem(const FSmartAssetCreateRequest& Request);
};
