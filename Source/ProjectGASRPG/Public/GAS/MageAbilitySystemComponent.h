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

	/* 向ASC授予所有GameplayAbility */
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);

	/* 输入时，激活标签匹配的Ability */
	void AbilityInputTagPressed(const FGameplayTag& InputTag); //Pressed时，激活标签匹配的Pressed Ability
	void AbilityInputTagHold(const FGameplayTag& InputTag);    //Hold时，激活标签匹配Hold的Ability
	void AbilityInputTagReleased(const FGameplayTag& InputTag);//Released时，激活标签匹配的Ability
protected:

	UFUNCTION(Client, Reliable)
	void EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;
};
