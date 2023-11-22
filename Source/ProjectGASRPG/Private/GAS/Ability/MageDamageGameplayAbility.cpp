#include "GAS/Ability/MageDamageGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Interface/CombatInterface.h"

void UMageDamageGameplayAbility::CauseDamage(AActor* TargetActor)
{
	const FGameplayEffectSpecHandle DamageEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());

	/**
	 * 使用Set By Caller Modifier 从曲线表格中获取技能类型伤害
	 * - AssignTagSetByCallerMagnitude 设置 DamageTypeTag 对应的 magnitude
	 * - 基于技能等级获取曲线表格的值
	 */
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageEffectSpecHandle, DamageTypeTag, TypeDamage.GetValueAtLevel(GetAbilityLevel())); 

	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*DamageEffectSpecHandle.Data.Get(), UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor));
}

float UMageDamageGameplayAbility::GetTypeDamage_Implementation(
	const int32 AbilityLevel) const
{
	return TypeDamage.GetValueAtLevel(AbilityLevel);
}


