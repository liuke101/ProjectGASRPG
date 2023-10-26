// 

#pragma once

#include "CoreMinimal.h"
#include "MageGameplayAbility.h"
#include "MageDamageGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMageDamageGameplayAbility : public UMageGameplayAbility
{
	GENERATED_BODY() 
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GE")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Damage")
	TMap<FGameplayTag, FScalableFloat> DamageTypes; //使用曲线表格控制技能伤害（随等级变化）
};
