#include "GAS/GameplayEffect/MMC/Ability/MMC_AbilityCost.h"

#include "GAS/Ability/MageGameplayAbility.h"

UMMC_AbilityCost::UMMC_AbilityCost()
{
}

float UMMC_AbilityCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UMageGameplayAbility* Ability = Cast<UMageGameplayAbility>(Spec.GetContext().GetAbilityInstance_NotReplicated());
	if (!Ability)
	{
		return 0.0f;
	}
	return Ability->Cost.GetValueAtLevel(Ability->GetAbilityLevel());
}
