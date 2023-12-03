#include "GAS/Ability/MageGameplayAbility.h"

#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"

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

const FGameplayTagContainer* UMageGameplayAbility::GetCooldownTags() const
{
	FGameplayTagContainer* MutableTags = const_cast<FGameplayTagContainer*>(&TempCooldownTags);
	const FGameplayTagContainer* ParentTags = Super::GetCooldownTags();
	if (ParentTags)
	{
		MutableTags->AppendTags(*ParentTags);
	}
	MutableTags->AppendTags(CooldownTags);
	return MutableTags;
}

void UMageGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE)
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
		SpecHandle.Data.Get()->DynamicGrantedTags.AppendTags(CooldownTags);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMageGameplayTags::Instance().Data_Cooldown, CooldownDuration.GetValueAtLevel(GetAbilityLevel()));
		// ReSharper disable once CppExpressionWithoutSideEffects
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
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
		float Duration = 0;
		CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(AbilityLevel,Duration);
		return Duration;
	}
	return -1;
}




