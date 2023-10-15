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

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagHold(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
protected:
	void EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;
};
