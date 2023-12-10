// 


#include "GAS/GameplayEffect/MMC/Ability/MMC_AbilityHealthCost.h"

#include "GAS/Ability/MageGameplayAbility.h"

UMMC_AbilityHealthCost::UMMC_AbilityHealthCost()
{
}

float UMMC_AbilityHealthCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UMageGameplayAbility* Ability = Cast<UMageGameplayAbility>(Spec.GetContext().GetAbilityInstance_NotReplicated());
	if (!Ability)
	{
		return 0.0f;
	}
	return Ability->HealthCost.GetValueAtLevel(Ability->GetAbilityLevel());
}
