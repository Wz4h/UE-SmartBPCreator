#pragma once

#include "CoreMinimal.h"
#include "Core/SmartAssetCreateRequest.h"

class USmartAssetSettings;

class SMARTASSETCREATOR_API FSmartAssetRuleResolver
{
public:
	static const USmartAssetSettings* GetSettings();
	static FString ResolvePrefix(const FSmartAssetCreateRequest& Request);
	static FString BuildAssetBaseName(const FSmartAssetCreateRequest& Request);
	static FString StripKnownPrefix(const FString& InName);

private:
	static FString ResolveBuiltInPrefix(const FSmartAssetCreateRequest& Request);
	static bool UsesClassRules(ESmartAssetType AssetType);
	static UClass* ResolveRuleClassCandidate(const FSmartAssetCreateRequest& Request);
	static int32 ComputeInheritanceDistance(const UClass* ChildClass, const UClass* AncestorClass);
	static FString BuildNameStem(const FSmartAssetCreateRequest& Request);
};
