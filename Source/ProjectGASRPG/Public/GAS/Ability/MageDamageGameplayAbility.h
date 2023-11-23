// 

#pragma once

#include "CoreMinimal.h"
#include "MageGameplayAbility.h"
#include "GAS/MageAbilityTypes.h"
#include "MageDamageGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMageDamageGameplayAbility : public UMageGameplayAbility
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void CauseDamage(AActor* TargetActor);

	FDamageEffectParams MakeDamageEffectParamsFromClassDefault(AActor* TargetActor = nullptr) const;
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	FScalableFloat TypeDamage; //使用曲线表格控制技能伤害（随等级变化）

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float DebuffChance = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float DebuffDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float DebuffFrequency = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float DebuffDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	FDamageEffectParams DamageEffectParams;
	
	/** 获取类型伤害 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|DamageAbility")
	float GetTypeDamage(const int32 AbilityLevel) const;
};
