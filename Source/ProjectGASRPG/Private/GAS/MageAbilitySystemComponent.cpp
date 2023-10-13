#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"

void UMageAbilitySystemComponent::BindEffectCallbacks()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UMageAbilitySystemComponent::EffectAppliedToSelfCallback);

	const FMageGameplayTags& GameplayTagsInstance = FMageGameplayTags::Get();
	//GameplayTags.Attribute_Secondary_MaxHealth.ToString()
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
	                                 FString::Printf(TEXT("Tags: %s"), *GameplayTagsInstance.Attribute_Secondary_MaxHealth.ToString()));
}

void UMageAbilitySystemComponent::EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	/* 广播 Tag 到WidgetController */  
	EffectAssetTags.Broadcast(TagContainer);
}
