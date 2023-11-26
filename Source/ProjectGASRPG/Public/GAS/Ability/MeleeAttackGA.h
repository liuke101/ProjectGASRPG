
#pragma once

#include "CoreMinimal.h"
#include "MageDamageGameplayAbility.h"
#include "MeleeAttackGA.generated.h"

UCLASS()
class PROJECTGASRPG_API UMeleeAttackGA : public UMageDamageGameplayAbility
{
	GENERATED_BODY()

	// 对应蓝图中的 Event ActivateAbility, 激活时调用
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
