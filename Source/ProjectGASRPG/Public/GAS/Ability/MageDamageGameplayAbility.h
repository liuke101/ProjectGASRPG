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

	/** 从类默认值中创建DamageEffectParams, 默认TargetActor为空, 需手动设置(例如在触发Overlap时将OtherActor设置为TargetActor) */
	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "Mage_GA|DamageAbility")
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

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float DeathImpulseMagnitude = 1000.0f; // 死亡冲量
	
	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float KnockbackForceMagnitude = 60.0f; // 击退力

	UPROPERTY(EditdefaultsOnly, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	float KnockbackChance = 0.2f; // 击退概率

	
	
	/** 获取类型伤害 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|DamageAbility")
	float GetTypeDamage(const int32 AbilityLevel) const;
};
