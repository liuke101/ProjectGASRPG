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

	/* 向ASC授予（Give）所有GameplayAbility, 将 GA 的 Tag 添加到AbilitySpec(目前仅玩家类调用) */
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& CharacterAbilities);

	/* 输入时，激活标签匹配的Ability */
	void AbilityInputTagPressed(const FGameplayTag& InputTag); //Pressed时，激活标签匹配的Pressed Ability
	void AbilityInputTagHold(const FGameplayTag& InputTag);    //Hold时，激活标签匹配Hold的Ability
	void AbilityInputTagReleased(const FGameplayTag& InputTag);//Released时，激活标签匹配的Ability
	
protected:

	UFUNCTION(Client, Reliable)
	void EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;
	
};
