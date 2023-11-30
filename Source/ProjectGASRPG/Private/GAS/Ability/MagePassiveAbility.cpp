// 


#include "GAS/Ability/MagePassiveAbility.h"

#include "GAS/MageAbilitySystemComponent.h"

void UMagePassiveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		MageASC->DeactivatePassiveAbility.AddUObject(this,&UMagePassiveAbility::DeactivatePassiveAbilityCallback);
	}
}

void UMagePassiveAbility::DeactivatePassiveAbilityCallback(const FGameplayTag& AbilityTag)
{
	if(AbilityTags.HasTagExact(AbilityTag))
	{
		EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);
	}
}
