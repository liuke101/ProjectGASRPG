// 


#include "GAS/MageAbilitySystemGlobals.h"

#include "GAS/MageAbilityTypes.h"

FGameplayEffectContext* UMageAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FMageGameplayEffectContext(); //返回自定义的MageGameplayEffectContext结构体
}
