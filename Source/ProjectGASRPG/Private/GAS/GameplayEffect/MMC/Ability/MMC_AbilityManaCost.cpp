#include "GAS/GameplayEffect/MMC/Ability/MMC_AbilityManaCost.h"
#include "GAS/Ability/MageGameplayAbility.h"

UMMC_AbilityManaCost::UMMC_AbilityManaCost()
{
}

float UMMC_AbilityManaCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UMageGameplayAbility* Ability = Cast<UMageGameplayAbility>(Spec.GetContext().GetAbilityInstance_NotReplicated());
	if (!Ability)
	{
		return 0.0f;
	}
	return Ability->ManaCost.GetValueAtLevel(Ability->GetAbilityLevel());
}
