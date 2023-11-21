﻿#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MageWidgetController.h"
#include "GAS/MageGameplayTags.h"
#include "SkillTreeWidgetController.generated.h"

class FMageGameplayTags;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillIconSelectedDelegate, bool, bLearnSkillButtonEnabled, bool,
                                             bEquipSkillButtonEnabled);

struct FSelectedAbility
{
	FGameplayTag AbilityTag = FGameplayTag();
	FGameplayTag StateTag = FGameplayTag();
};

UCLASS()
class PROJECTGASRPG_API USkillTreeWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/**
     * 广播初始值，供 SkillTree 初始化
     * 在 SetWidgetController() 之后调用
     */
	virtual void BroadcastInitialValue() override;

	/**
	 * 绑定委托回调
	 * GetSkillTreeWidgetController() 中调用
	 */
	virtual void BindCallbacks() override;

	/** 等级数据变化委托，由WBP_SkillTreeMenu监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnSkillPointChangedDelegate;

	/** 技能图标选中委托, 由SkillTreeIcon监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnSkillIconSelectedDelegate CheckButtonEnableOnSkillIconSelected;
	
	/** 点击SkillTreeIcon时触发，根据AbilityStateTag判断学习技能和装备技能按钮是否开启，并广播OnSkillIconSelectedDelegate，将是否开启的bool值广播到控件 */
	UFUNCTION(BlueprintCallable, Category = "Mage_SkillTree")
	void SkillIconSelected(const FGameplayTag& AbilityTag);

private:
	/** 根据AbilityStateTag判断学习技能和装备技能按钮是否开启 */
	static void ShouldEnableButton(const FGameplayTag& AbilityStateTag, int32 SkillPoint, bool& bLearnSkillButtonEnabled, bool& bEquipSkillButtonEnabled);

	/** 保存选中技能状态 */
	FSelectedAbility SelectedAbility{FMageGameplayTags::Get().Ability_None, FMageGameplayTags::Get().Ability_State_Locked};
	int32 CurrentSkillPoint = 0;
};
