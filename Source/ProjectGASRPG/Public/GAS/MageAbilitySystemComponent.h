#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MageAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FEffectAssetTagsDelegates, const FGameplayTagContainer& /*AssetTags*/);	

UCLASS()
class PROJECTGASRPG_API UMageAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void BindEffectCallbacks();

	/* GameplayEffectApplyToSelf 时广播，用于将 GameplayTagContainer 传递给 WidgetController */
	FEffectAssetTagsDelegates EffectAssetTags;
protected:
	void EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle);
};
