#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MageGameplayAbility.generated.h"


UCLASS()
class PROJECTGASRPG_API UMageGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UMageGameplayAbility();

public:
	UPROPERTY(EditAnywhere, Category = "Mage_GA")
	int32 StartupAbilityLevel = 1; //技能初始等级(默认为1即可)
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Input")
	FGameplayTag StartupInputTag;

	/** 用于SkillTreeMenu获取技能描述, 子类可以重载它 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA")
	FString GetDescription(const int32 AbilityLevel);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA")
	FString GetNextLevelDescription(const int32 AbilityLevel);
	static FString GetLockedDescription(const int32 LevelRequirement);

protected:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA")
	float GetManaCost(const int32 AbilityLevel) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA")
	float GetCooldown(const int32 AbilityLevel) const;
};
