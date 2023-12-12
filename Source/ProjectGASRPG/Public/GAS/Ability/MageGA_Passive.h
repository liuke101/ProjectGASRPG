// 

#pragma once

#include "CoreMinimal.h"
#include "MageGameplayAbility.h"
#include "MageGA_Passive.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMageGA_Passive : public UMageGameplayAbility
{
	GENERATED_BODY()
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	void DeactivatePassiveAbilityCallback(const FGameplayTag& AbilityTag);
};
