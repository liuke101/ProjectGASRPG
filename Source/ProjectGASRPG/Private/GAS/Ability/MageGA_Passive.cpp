// 


#include "GAS/Ability/MageGA_Passive.h"

#include "GAS/MageAbilitySystemComponent.h"

void UMageGA_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		MageASC->DeactivatePassiveAbility.AddUObject(this,&UMageGA_Passive::DeactivatePassiveAbilityCallback);
	}
}

void UMageGA_Passive::DeactivatePassiveAbilityCallback(const FGameplayTag& AbilityTag)
{
	if(AbilityTags.HasTagExact(AbilityTag))
	{
		EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);
	}
}
