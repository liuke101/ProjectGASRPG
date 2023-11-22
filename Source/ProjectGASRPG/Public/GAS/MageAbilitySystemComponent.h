#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MageAbilitySystemComponent.generated.h"

class UMageAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectAppliedToSelfDelegates, const FGameplayTagContainer& /*AssetTags*/);
DECLARE_MULTICAST_DELEGATE(FOnGiveCharacterAbilities);
DECLARE_DELEGATE_OneParam(FForEachAbilityDelegate, const FGameplayAbilitySpec& /*GASpec*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAbilityStateChanged, const FGameplayTag& /*AbilityTag*/, const FGameplayTag& /*AbilityStateTag*/,int32 /*AbilityLevel*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnSkillEquipped, const FGameplayTag& /*AbilityTag*/, const FGameplayTag& /*AbilityStateTag*/, const FGameplayTag& /*SlotInputTag*/, const FGameplayTag& /*PreSlotInputTag*/);

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

	/** 从GASpec对应的GA的 GATagContainer 中获取 */
	static FGameplayTag GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec); // 获取匹配 "Ability" 的Tag
	static FGameplayTag GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec); // 获取匹配 "Input" 的Tag
	static FGameplayTag GetStateTagFromSpec(const FGameplayAbilitySpec& AbilitySpec); // 获取匹配 "State" 的Tag

	/** 由 AbilityTag 获取  */
	FGameplayAbilitySpec* GetSpecFromAbilityTag(const FGameplayTag& AbilityTag); // 获取 AbilitySpec
	FGameplayTag GetInputTagFromAbilityTag(const FGameplayTag& AbilityTag); // 获取 InputTag
	FGameplayTag GetStateTagFromAbilityTag(const FGameplayTag& AbilityTag); // 获取StateTag
	
	/* GameplayEffectApplyToSelf 时广播，用于将 GameplayTagContainer 传到 OverlayWidgetController */
	FOnEffectAppliedToSelfDelegates EffectAssetTags;

	/* 当Ability被授予时广播，用于将 Ability信息 传到 WidgetController */
	FOnGiveCharacterAbilities AbilitiesGiven;

	/* 当AbilityState发生变化时广播 */
	FOnAbilityStateChanged AbilityStateChanged;

	/* 当装备技能时广播, 将技能 Tag 信息广播到WBP_EquippedSkillTree */
	FOnSkillEquipped SkillEquipped;

	/** 升级属性，只在服务器执行 */
	void UpgradeAttribute(const FGameplayTag& AttributeTag);
	void ServerUpgradeAttribute(const FGameplayTag& AttributeTag);

	/** 升级时更新AbilityState */
	void UpdateAbilityState(int32 Level);

	/** 学习技能（消耗技能点，提升技能等级，更新技能状态） */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLearnSkill(const FGameplayTag& AbilityTag);

	/** 根据AbilityTag获取技能描述 */
	bool GetDescriptionByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription,FString& OutNextLevelDescription);

	/** 装备技能 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipSkill(const FGameplayTag& AbilityTag, const FGameplayTag& SlotInputTag);
	
	UFUNCTION(Client, Reliable, WithValidation)
	void ClientEquipSkill(const FGameplayTag& AbilityTag, const FGameplayTag& AbilityStateTag, const FGameplayTag& SlotInputTag,const FGameplayTag& PreSlotInputTag);
	
	/** 清除指定Ability的 SlotInputTag */
	void ClearSlotInputTag(FGameplayAbilitySpec* Spec);
	/** 清除所有授予的Ability的 SlotInputTag */
	void ClearAbilityOfSlotInputTag(const FGameplayTag& SlotInputTag);
	/** 判断Ability是否拥有 SlotInputTag */
	static bool AbilityHasSlotInputTag(FGameplayAbilitySpec* Spec, const FGameplayTag& SlotInputTag);
protected:
	virtual void OnRep_ActivateAbilities() override;
	
	UFUNCTION(Client, Reliable)
	void ClientEffectAppliedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAbilityState(const FGameplayTag& AbilityTag, const FGameplayTag& StateTag, int32 AbilityLevel) const;
};
