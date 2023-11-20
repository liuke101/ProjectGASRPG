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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag CooldownTag = FGameplayTag();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UTexture2D> AbilityIconImage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UTexture2D> AbilityTypeImage = nullptr;

	/** InputTag 用于绑定输入 */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag InputTag = FGameplayTag(); //不通过编辑器设置，在 BroadcastAbilityInfo() 回调中设置

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag StateTag = FGameplayTag(); //由于动态改变，所以不通过编辑器设置，在 BroadcastAbilityInfo() 回调中设置
	
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
