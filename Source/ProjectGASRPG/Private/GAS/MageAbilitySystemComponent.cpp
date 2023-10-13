#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"

void UMageAbilitySystemComponent::BindEffectCallbacks()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::EffectAppliedToSelfCallback);

}

void UMageAbilitySystemComponent::EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	/* 广播 Tag 到WidgetController */  
	EffectAssetTags.Broadcast(TagContainer);
}
