// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "MageAbilitySystemGlobals.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMageAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	/** 创建一个新的FMageGameplayEffectContext让ASC使用, 在 UAbilitySystemComponent::MakeEffectContext() 中调用 */
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
	
};
