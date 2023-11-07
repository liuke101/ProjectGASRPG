﻿// 

#pragma once

#include "CoreMinimal.h"
#include "MageGameplayAbility.h"
#include "MageDamageGameplayAbility.generated.h"

struct FTaggedMontage;
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
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Damage")
	TMap<FGameplayTag, FScalableFloat> DamageTypeTag_To_AbilityDamage; //使用曲线表格控制技能伤害（随等级变化）
};
