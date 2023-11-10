// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AbilityDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FMageAbilityInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AbilityTag = FGameplayTag();

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag InputTag = FGameplayTag(); //不通过编辑器设置，在 OnInitializeStartupAbilities() 回调中设置

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UTexture2D> AbilityIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UTexture2D> AbilityType = nullptr;
};

UCLASS()
class PROJECTGASRPG_API UAbilityDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	FMageAbilityInfo FindAbilityInfoForTag(const FGameplayTag& AbilityTag, bool blogNotFound = false) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS_AbilityInfo")
	TArray<FMageAbilityInfo> AbilityInfos;

};
