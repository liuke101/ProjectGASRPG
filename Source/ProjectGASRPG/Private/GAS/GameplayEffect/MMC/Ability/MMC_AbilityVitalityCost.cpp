#include "GAS/GameplayEffect/MMC/Ability/MMC_AbilityVitalityCost.h"

#include "GAS/Ability/MageGameplayAbility.h"

UMMC_AbilityVitalityCost::UMMC_AbilityVitalityCost()
{
}

float UMMC_AbilityVitalityCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UMageGameplayAbility* Ability = Cast<UMageGameplayAbility>(Spec.GetContext().GetAbilityInstance_NotReplicated());
	if (!Ability)
	{
		return 0.0f;
	}
	return Ability->VitalityCost.GetValueAtLevel(Ability->GetAbilityLevel());
}
