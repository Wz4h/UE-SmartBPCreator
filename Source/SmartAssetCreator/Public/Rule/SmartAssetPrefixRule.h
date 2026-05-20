#pragma once

#include "CoreMinimal.h"
#include "SmartAssetPrefixRule.generated.h"

USTRUCT()
struct SMARTASSETCREATOR_API FSmartAssetPrefixRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = "Rule", meta = (AllowedClasses = "/Script/CoreUObject.Object"))
	FSoftClassPath BaseClass;

	UPROPERTY(EditAnywhere, Config, Category = "Rule")
	FString Prefix;

	UPROPERTY(EditAnywhere, Config, Category = "Rule")
	bool bIncludeDerivedClasses = true;

	UPROPERTY(EditAnywhere, Config, Category = "Rule")
	bool bEnabled = true;
};
