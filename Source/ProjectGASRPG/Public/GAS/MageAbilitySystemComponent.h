#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MageAbilitySystemComponent.generated.h"

class UMageAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectAppliedToSelfDelegates, const FGameplayTagContainer& /*AssetTags*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGiveCharacterAbilities, UMageAbilitySystemComponent* /*ASC*/);
DECLARE_DELEGATE_OneParam(FForEachAbilityDelegate, const FGameplayAbilitySpec& /*GASpec*/);

UCLASS()
class PROJECTGASRPG_API UMageAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/* 输入时，激活标签匹配的Ability */
	void AbilityInputTagPressed(const FGameplayTag& InputTag); //Pressed时，激活标签匹配的Pressed Ability
	void AbilityInputTagHold(const FGameplayTag& InputTag);    //Hold时，激活标签匹配Hold的Ability
	void AbilityInputTagReleased(const FGameplayTag& InputTag);//Released时，激活标签匹配的Ability
	
	void BindEffectCallbacks();

	/* 向ASC授予（Give）所有GameplayAbility, 将 GA 的 Tag 添加到AbilitySpec(目前仅玩家类调用) */
	void GiveCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& CharacterAbilities);
	bool bStartupAbilitiesGiven = false; //是否已经授予了Ability

	/**
	 * 遍历可激活的Ability, 将所有可激活的Ability作为单播委托参数执行
	 * - 通过在ASC组件内部执行，可以给激活列表加锁，避免在外部（如WidgetController）进行不安全的遍历
	 */
	void  ForEachAbility(const FForEachAbilityDelegate& AbilityDelegate);

	/** 从GASpec对应的GA的 GATagContainer 中获取匹配 "Ability" 的Tag */
	static FGameplayTag GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	static FGameplayTag GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	
	/* GameplayEffectApplyToSelf 时广播，用于将 GameplayTagContainer 传到 OverlayWidgetController */
	FOnEffectAppliedToSelfDelegates EffectAssetTags;

	/* 当Ability被授予时广播，用于将 Ability信息 传到 OverlayWidgetController */
	FOnGiveCharacterAbilities AbilitiesGiven;

protected:

	UFUNCTION(Client, Reliable)
	void EffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;

	
};
