#include "UI/WidgetController/OverlayWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"

void UOverlayWidgetController::BrodCastInitialValue()
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

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetHealthAttribute()).AddUObject(this, &UOverlayWidgetController::OnHealthChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxHealthAttribute()).AddUObject(this, &UOverlayWidgetController::OnMaxHealthChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetManaAttribute()).AddUObject(this, &UOverlayWidgetController::OnManaChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxManaAttribute()).AddUObject(this, &UOverlayWidgetController::OnMaxManaChangedCallback);

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

void UOverlayWidgetController::OnHealthChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnMaxHealthChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnManaChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnManaChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnMaxManaChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnMaxManaChanged.Broadcast(Data.NewValue);
}
