#include "GAS/MageAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/MageCharacter.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"



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

void UMageAbilitySystemComponent::BindEffectCallbacks()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::ClientEffectAppliedToSelfCallback);
}

void UMageAbilitySystemComponent::GiveCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& CharacterAbilities)
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
		}
	}

	/**
	 * 将所有授予的Ability的 AbilityInfo 广播给 OverlayWidgetController
	 *  - 由于GiveCharacterAbilities函数只在服务器运行, 我们重载 OnRep_ActivateAbilities() 实现在客户端广播
	 */
	bCharacterAbilitiesGiven = true;
	AbilitiesGiven.Broadcast();
}

void UMageAbilitySystemComponent::GivePassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassiveAbilities)
{
	for (const auto AbilityClass : PassiveAbilities)
	{
		
		FGameplayAbilitySpec AbilitySpec(AbilityClass); 
		
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->AbilityLevel; //设置技能等级
			
			/** 授予后立即激活一次（被动技能持续激活，激活一次但不EndAbility） */
			GiveAbilityAndActivateOnce(AbilitySpec); 
		}
	}
}

void UMageAbilitySystemComponent::ForEachAbility(const FForEachAbilityDelegate& AbilityDelegate)
{
	/** 注意：Ability在运行时可以改变状态，例如取消激活，被某个Tag block。因此循环【激活列表】时, 要锁定该列表，直到循环完成 */
	FScopedAbilityListLock ActiveScopeLock(*this);
	
	//遍历可激活的Ability
	for(const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		//执行委托,AbilitySpec为参数
		if(!AbilityDelegate.ExecuteIfBound(AbilitySpec))
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,FString::Printf(TEXT("未能执行委托: %hs"), __FUNCTION__));
		}
	}
}

FGameplayTag UMageAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if(UGameplayAbility* Ability = AbilitySpec.Ability.Get())
	{
		for(FGameplayTag AbilityTag : Ability->AbilityTags)
		{
			if(AbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Ability"))))
			{
				return AbilityTag;
			}
		}
	}
	return FGameplayTag::EmptyTag;
}

FGameplayTag UMageAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for(FGameplayTag InputTag : AbilitySpec.DynamicAbilityTags) //DynamicAbilityTags 在 GiveCharacterAbilities时添加
	{
		if(InputTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Input"))))
		{
			return InputTag;
		}
	}
	return FGameplayTag::EmptyTag;
}

void UMageAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if(const IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		// 如果有属性点
		if(PlayerInterface->GetAttributePoint() > 0 )
		{
			ServerUpgradeAttribute(AttributeTag); // 服务器执行升级属性
		}
	}
}

void UMageAbilitySystemComponent::ServerUpgradeAttribute(const FGameplayTag& AttributeTag)
{
	// 使用GameplayEvent升级属性
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(),AttributeTag,Payload);

	// 减少属性点
	if(IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(GetAvatarActor()))
	{
		PlayerInterface->AddToAttributePoint(-1);
	}
}

void UMageAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	if(!bCharacterAbilitiesGiven)
	{
		bCharacterAbilitiesGiven = true;
		AbilitiesGiven.Broadcast();
	}
}

void UMageAbilitySystemComponent::ClientEffectAppliedToSelfCallback_Implementation(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer); 
	
	/* 广播 Tag 到 OverlayWidgetController */
	EffectAssetTags.Broadcast(TagContainer);

	//debug
	for (const FGameplayTag& Tag : TagContainer)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Green, FString::Printf(TEXT("Effect Applied! GE Tag: %s"), *Tag.ToString()));
	}
}
