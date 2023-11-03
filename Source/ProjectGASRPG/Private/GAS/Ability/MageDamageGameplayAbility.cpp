#include "GAS/Ability/MageDamageGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void UMageDamageGameplayAbility::CauseDamage(AActor* TargetActor)
{
	const FGameplayEffectSpecHandle DamageEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());

	//使用Set By Caller Modifier 从曲线表格中获取技能类型伤害
	for(auto& Pair:DamageTypeTag_To_AbilityDamage)
	{
		const float TypeDamage = Pair.Value.GetValueAtLevel(GetAbilityLevel()); //基于技能等级获取曲线表格的值
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageEffectSpecHandle, Pair.Key, TypeDamage); //设置Tag对应的magnitude
	}

	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*DamageEffectSpecHandle.Data.Get(), UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor));
}
