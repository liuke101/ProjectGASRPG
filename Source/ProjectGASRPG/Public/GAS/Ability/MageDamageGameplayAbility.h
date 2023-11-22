// 

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA|DamageAbility")
	TMap<FGameplayTag, FScalableFloat> DamageTypeTag_To_AbilityDamage; //使用曲线表格控制技能伤害（随等级变化）

	/** 获取类型伤害 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|DamageAbility")
	float GetTypeDamage(const FGameplayTag& DamageTypeTag,const int32 AbilityLevel) const;
	
};
