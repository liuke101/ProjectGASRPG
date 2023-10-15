#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"

void UMageAbilitySystemComponent::BindEffectCallbacks()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::EffectAppliedToSelfCallback);
}

void UMageAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (auto AbilityClass : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);

		/** 添加动态 Tag 到AbilitySpec */
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.DynamicAbilityTags.AddTag(MageGameplayAbility->StartupInputTag);
			/** 授予Ability */
			GiveAbility(AbilitySpec); //授予后不激活
			//GiveAbilityAndActivateOnce(AbilitySpec); //授予并立即激活一次
			
		}
	}
}

void UMageAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	// if (!InputTag.IsValid()) return;
	//
	// for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	// {
	// 	if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) //标签匹配
	// 	{
	// 		AbilitySpecInputPressed(AbilitySpec); // 通知AbilitySpec输入被按下
	// 		if (!AbilitySpec.IsActive()) //如果Ability没有激活
	// 		{
	// 			TryActivateAbility(AbilitySpec.Handle); //激活Ability
	// 		}
	// 	}
	// }
}

void UMageAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) //标签匹配
		{
			AbilitySpecInputReleased(AbilitySpec); // 通知AbilitySpec输入被释放
			if (AbilitySpec.IsActive()) //如果Ability已经激活
			{
				
			}
		}
	}
}


void UMageAbilitySystemComponent::AbilityInputTagHold(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) //标签匹配
		{
			AbilitySpecInputPressed(AbilitySpec); // 通知AbilitySpec输入被按下
			if (!AbilitySpec.IsActive()) //如果Ability没有激活
			{
				TryActivateAbility(AbilitySpec.Handle); //激活Ability
			}
		}
	}
}

void UMageAbilitySystemComponent::EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC,
                                                              const FGameplayEffectSpec& EffectSpec,
                                                              FActiveGameplayEffectHandle ActiveEffectHandle) const
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	/* 广播 Tag 到WidgetController */
	EffectAssetTags.Broadcast(TagContainer);
}
