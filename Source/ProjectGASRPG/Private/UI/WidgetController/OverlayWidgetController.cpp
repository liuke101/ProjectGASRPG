﻿#include "UI/WidgetController/OverlayWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/Data/AbilityDataAsset.h"

void UOverlayWidgetController::BroadcastInitialValue()
{
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);

	OnHealthChanged.Broadcast(MageAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(MageAttributeSet->GetMaxHealth());
	OnManaChanged.Broadcast(MageAttributeSet->GetMana());
	OnMaxManaChanged.Broadcast(MageAttributeSet->GetMaxMana());
	OnVitalityChanged.Broadcast(MageAttributeSet->GetVitality());
	OnMaxVitalityChanged.Broadcast(MageAttributeSet->GetMaxVitality());
}

void UOverlayWidgetController::BindCallbacks()
{
	/** 绑定ASC属性变化回调，接收属性变化 */
	if(const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnManaChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChanged.Broadcast(Data.NewValue);
		});

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetVitalityAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnVitalityChanged.Broadcast(Data.NewValue);
		});

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxVitalityAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxVitalityChanged.Broadcast(Data.NewValue);
		});
	}
	
	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(AbilitySystemComponent))
	{
		/** 
		 * 绑定 AbilitiesGiven 委托
		 * - 如果已经授予了Ability，可以直接执行回调
		 * - 否则绑定委托回调，等待GiveCharacterAbilities()执行
		 */
		if(MageASC->bStartupAbilitiesGiven)
		{
			OnInitializeStartupAbilities(MageASC);
		}
		else 
		{
			MageASC->AbilitiesGiven.AddUObject(this, &UOverlayWidgetController::OnInitializeStartupAbilities);
		}
		
		/** 绑定 EffectAssetTags 回调，接收 GameplayTagContainer */
		MageASC->EffectAssetTags.AddLambda([this](const FGameplayTagContainer& AssetTags)
		{
			for (auto& Tag : AssetTags)
			{
				/**
				 * "A.B".MatchesTag("A") 返回 True, "A".MatchesTag("A.B") 返回 False
				 * 若TagtoCheck无效总是返回 False
				 *
				 * 例如，Tag = "Message.HealthPotion"，则 Tag.MatchesTag("Message") 返回 True
				 */
				FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
				if (Tag.MatchesTag(MessageTag))
				{
					if (const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag))
					{
						MessageWidgetRowDelegate.Broadcast(*Row); //广播数据表行, 在 OverlayUserWidget 蓝图中绑定该委托
					}
				}
			}
		});
	}
}

void UOverlayWidgetController::OnInitializeStartupAbilities(UMageAbilitySystemComponent* MageASC)
{
	/** 获取所有授予的Ability, 查询AbilityDataAsset, 将他们广播给OverlayUserWidget */
	if(!MageASC->bStartupAbilitiesGiven) return;

	FForEachAbilityDelegate AbilityDelegate;
	// 绑定AbilityDelegate委托
	AbilityDelegate.BindLambda([this, MageASC](const FGameplayAbilitySpec& AbilitySpec)
	{
		
		FMageAbilityInfo Info = AbilityDataAsset->FindAbilityInfoForTag(MageASC->GetAbilityTagFromSpec(AbilitySpec));
		Info.InputTag = MageASC->GetInputTagFromSpec(AbilitySpec);

		// 注意这是另一个委托，负责将 AbilityInfo 广播给 UserWidget
		AbilityInfoDelegate.Broadcast(Info);
	});

	/** 执行AbilityDelegate委托, 结合委托绑定的Lambda，同时也将AbilityInfoDelegate委托广播。 */
	/** 这样所有被授予的Ability都会将信息被广播给OverlayUserWidget */
	
	MageASC->ForEachAbility(AbilityDelegate);
}




