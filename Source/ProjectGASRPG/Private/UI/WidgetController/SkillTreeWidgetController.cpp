// 


#include "UI/WidgetController/SkillTreeWidgetController.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/Data/AbilityDataAsset.h"
#include "Player/MagePlayerState.h"

void USkillTreeWidgetController::BroadcastInitialValue()
{
	OnSkillPointChangedDelegate.Broadcast(GetMagePlayerState()->GetSkillPoint());
	
	BroadcastAbilityInfo();
}

void USkillTreeWidgetController::BindCallbacks()
{
	/** 
	 * 绑定 AbilitiesGiven 委托
	 * - 如果已经授予了Ability，可以直接执行回调
	 * - 否则绑定委托回调，等待GiveCharacterAbilities()执行
	 */
	if(GetMageASC()->bCharacterAbilitiesGiven)
	{
		BroadcastAbilityInfo();
	}
	else 
	{
		GetMageASC()->AbilitiesGiven.AddUObject(this, &USkillTreeWidgetController::BroadcastAbilityInfo);
	}

	/** 绑定 AbilityStateChanged 委托 */
	GetMageASC()->AbilityStateChanged.AddLambda([this](const FGameplayTag& AbilityTag, const FGameplayTag& AbilityStateTag)
	{
		if(AbilityDataAsset == nullptr) return;

		// 根据AbilityTag获取对应的AbilityInfo
		FMageAbilityInfo Info = AbilityDataAsset->FindAbilityInfoForTag(AbilityTag);
		Info.StateTag = AbilityStateTag; // 设置StateTag
		AbilityInfoDelegate.Broadcast(Info);
	});

	/** 等级变化->技能点变化 */
	GetMagePlayerState()->OnPlayerSkillPointChanged.AddLambda([this](const int32 NewSkillPoint)
	{
		OnSkillPointChangedDelegate.Broadcast(NewSkillPoint); //  广播技能点，在WBP_ExperienceBar中绑定 
	});
}
