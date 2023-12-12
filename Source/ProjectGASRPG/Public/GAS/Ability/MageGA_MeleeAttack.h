
#pragma once

#include "CoreMinimal.h"
#include "MageGA_Damage.h"
#include "MageGA_MeleeAttack.generated.h"

UCLASS()
class PROJECTGASRPG_API UMageGA_MeleeAttack : public UMageGA_Damage
{
	GENERATED_BODY()

	// 对应蓝图中的 Event ActivateAbility, 激活时调用
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
