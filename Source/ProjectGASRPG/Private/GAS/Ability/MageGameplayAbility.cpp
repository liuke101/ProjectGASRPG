#include "GAS/Ability/MageGameplayAbility.h"

#include "GAS/MageAttributeSet.h"

UMageGameplayAbility::UMageGameplayAbility()
{
	bServerRespectsRemoteAbilityCancellation = false;
}

FString UMageGameplayAbility::GetDescription_Implementation(const int32 AbilityLevel)
{
	return FString::Printf(TEXT("<MainBody>%s</>\n<MainBody>技能等级：</><Level>%d</>"),L"test",AbilityLevel);
}

FString UMageGameplayAbility::GetNextLevelDescription_Implementation(const int32 AbilityLevel)
{
	return GetDescription(AbilityLevel+1);
}

FString UMageGameplayAbility::GetLockedDescription(const int32 LevelRequirement)
{
	return FString::Printf(TEXT("<MainBody>技能解锁等级：</><Level>%d</>"),LevelRequirement);
}

float UMageGameplayAbility::GetManaCost_Implementation(const int32 AbilityLevel) const
{
	if(const UGameplayEffect* CostEffect = GetCostGameplayEffect())
	{
		for(FGameplayModifierInfo ModifierInfo : CostEffect->Modifiers)
		{
			if(ModifierInfo.Attribute == UMageAttributeSet::GetManaAttribute())
			{
				float ManaCost = 0.f;
				ModifierInfo.ModifierMagnitude.GetStaticMagnitudeIfPossible(AbilityLevel,ManaCost);
				return -ManaCost; //负负得正
			}
		}
	}
	return -1;
}

float UMageGameplayAbility::GetCooldown_Implementation(const int32 AbilityLevel) const
{
	if(const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect())
	{
		float CooldownDuration = 0;
		CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(AbilityLevel,CooldownDuration);
		return CooldownDuration;
	}
	return -1;
}




