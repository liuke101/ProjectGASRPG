#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"

void UMageAbilitySystemComponent::BindEffectCallbacks()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::EffectAppliedToSelfCallback);
}

void UMageAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& CharacterAbilities)
{
	for (const auto AbilityClass : CharacterAbilities)
	{
		//AbilityClass转化为MageGameplayAbility
		FGameplayAbilitySpec AbilitySpec(AbilityClass); 
		
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->AbilityLevel; //设置技能等级
			
			/** 将 GA 的 Tag 添加到AbilitySpec, 这些 Tag 将与输入的 Tag 进行匹配*/
			AbilitySpec.DynamicAbilityTags.AddTag(MageGameplayAbility->StartupInputTag);
			
			/** 授予Ability */
			GiveAbility(AbilitySpec); //授予后不激活
			//GiveAbilityAndActivateOnce(AbilitySpec); //授予并立即激活一次
		}
	}
}

void UMageAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
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

void UMageAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (auto& AbilitySpec : GetActivatableAbilities()) //遍历可激活的Ability
	{
		if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)) //标签匹配
		{
			AbilitySpecInputReleased(AbilitySpec); // 通知AbilitySpec输入被释放
		}
	}
}

void UMageAbilitySystemComponent::EffectAppliedToSelfCallback_Implementation(UAbilitySystemComponent* ASC,
	const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const
{
	GEngine->AddOnScreenDebugMessage(1, 8.f, FColor::Blue,FString("Effect Applied!"));

	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer); //TODO:多人有问题，这里TagContainer总是为空
	
	for (const FGameplayTag& Tag : TagContainer)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Blue, FString::Printf(TEXT("GE Tag: %s"), *Tag.ToString()));
	}
	
	/* 广播 Tag 到WidgetController */
	EffectAssetTags.Broadcast(TagContainer);
}
