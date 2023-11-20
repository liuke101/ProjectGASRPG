#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MageAbilitySystemComponent.generated.h"

class UMageAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectAppliedToSelfDelegates, const FGameplayTagContainer& /*AssetTags*/);
DECLARE_MULTICAST_DELEGATE(FOnGiveCharacterAbilities);
DECLARE_DELEGATE_OneParam(FForEachAbilityDelegate, const FGameplayAbilitySpec& /*GASpec*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAbilityStateChanged, const FGameplayTag& /*AbilityTag*/, const FGameplayTag& /*AbilityStateTag*/);

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
	void GivePassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& PassiveAbilities);
	bool bCharacterAbilitiesGiven = false; //是否已经授予了Ability
	
	/**
	 * 遍历所有可激活的Ability, 将所有可激活的Ability作为单播委托参数执行AbilityDelegate
	 * - 通过在ASC组件内部执行，可以给激活列表加锁，避免在外部（如WidgetController）进行不安全的遍历
	 */
	void  ForEachAbility(const FForEachAbilityDelegate& AbilityDelegate);

	/** 从GASpec对应的GA的 GATagContainer 中获取匹配 "Ability" 的Tag */
	static FGameplayTag GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	/** 从GASpec对应的GA的 GATagContainer 中获取匹配 "Input" 的Tag */
	static FGameplayTag GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	/** 从GASpec对应的GA的 GATagContainer 中获取匹配 "State" 的Tag */
	static FGameplayTag GetStateTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);

	FGameplayAbilitySpec* GetSpecFromAbilityTag(const FGameplayTag& AbilityTag);
	
	/* GameplayEffectApplyToSelf 时广播，用于将 GameplayTagContainer 传到 OverlayWidgetController */
	FOnEffectAppliedToSelfDelegates EffectAssetTags;

	/* 当Ability被授予时广播，用于将 Ability信息 传到 WidgetController */
	FOnGiveCharacterAbilities AbilitiesGiven;

	/* 当AbilityState发生变化时广播，用于将 Tag 传到 WidgetController */
	FOnAbilityStateChanged AbilityStateChanged;

	/** 升级属性，只在服务器执行 */
	void UpgradeAttribute(const FGameplayTag& AttributeTag);
	void ServerUpgradeAttribute(const FGameplayTag& AttributeTag);

	/** 升级时更新AbilityState */
	void UpdateAbilityState(int32 Level);
protected:
	virtual void OnRep_ActivateAbilities() override;
	
	UFUNCTION(Client, Reliable)
	void ClientEffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAbilityState(const FGameplayTag& AbilityTag, const FGameplayTag& StateTag) const;
};
