#include "UI/WidgetController/OverlayWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"

void UOverlayWidgetController::BroadcastInitialValue()
{
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);

	OnHealthChanged.Broadcast(MageAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(MageAttributeSet->GetMaxHealth());
	OnManaChanged.Broadcast(MageAttributeSet->GetMana());
	OnMaxManaChanged.Broadcast(MageAttributeSet->GetMaxMana());
}

void UOverlayWidgetController::BindCallbacks()
{
	/** 绑定属性变化回调，接收属性变化 */
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);

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

	/** 绑定 EffectAssetTags 回调，接收 GameplayTagContainer */
	Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->EffectAssetTags.AddLambda(
		[this](const FGameplayTagContainer& AssetTags)
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
		}

	);
}



