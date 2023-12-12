#include "GAS/Ability/MageGA_Damage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Interface/CombatInterface.h"

void UMageGA_Damage::CauseDamage(AActor* TargetActor)
{
	const FGameplayEffectSpecHandle DamageEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	
	/**
	 * 使用Set By Caller Modifier 从曲线表格中获取技能类型伤害
	 * - AssignTagSetByCallerMagnitude 设置 DamageTypeTag 对应的 magnitude
	 * - 基于技能等级获取曲线表格的值
	 */
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageEffectSpecHandle, DamageTypeTag, TypeDamage.GetValueAtLevel(GetAbilityLevel())); 

	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*DamageEffectSpecHandle.Data, UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor));
}

FDamageEffectParams UMageGA_Damage::MakeDamageEffectParamsFromClassDefault(AActor* TargetActor) const
{
	FDamageEffectParams Params;
	Params.WorldContextObject = GetAvatarActorFromActorInfo();
	Params.DamageGameplayEffectClass = DamageEffectClass;
	Params.SourceASC = GetAbilitySystemComponentFromActorInfo();
	Params.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	Params.AbilityLevel = GetAbilityLevel();
	Params.BaseDamage = TypeDamage.GetValueAtLevel(Params.AbilityLevel);
	Params.DamageTypeTag = DamageTypeTag;
	Params.DebuffChance = DebuffChance;
	Params.DebuffDamage = DebuffDamage;
	Params.DebuffFrequency = DebuffFrequency;
	Params.DebuffDuration = DebuffDuration;
	Params.DeathImpulseMagnitude = DeathImpulseMagnitude;
	Params.KnockbackForceMagnitude = KnockbackForceMagnitude;
	Params.KnockbackChance = KnockbackChance;

	if(IsValid(TargetActor))
	{
		FRotator Rotation =  (TargetActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation()).ToOrientationRotator();
		Rotation.Pitch = 45.0f;
		const FVector ToTarget = Rotation.Vector();

		Params.DeathImpulse = ToTarget * DeathImpulseMagnitude;
		Params.KnockbackForce = ToTarget * KnockbackForceMagnitude;
	}

	return Params;
}

void UMageGA_Damage::GetTargetingActorInfo()
{
	TargetingActor = UMageAbilitySystemLibrary::GetTargetingActor(GetAvatarActorFromActorInfo());
	if(TargetingActor)
	{
		TargetingActorLocation = TargetingActor->GetActorLocation();
	}
	else
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}


float UMageGA_Damage::GetTypeDamage_Implementation(
	const int32 AbilityLevel) const
{
	return TypeDamage.GetValueAtLevel(AbilityLevel);
}


