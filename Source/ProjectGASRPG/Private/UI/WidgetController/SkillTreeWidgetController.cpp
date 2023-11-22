// 


#include "UI/WidgetController/SkillTreeWidgetController.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"
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
	if (GetMageASC()->bCharacterAbilitiesGiven)
	{
		BroadcastAbilityInfo();
	}
	else
	{
		GetMageASC()->AbilitiesGiven.AddUObject(this, &USkillTreeWidgetController::BroadcastAbilityInfo);
	}

	/** 绑定 AbilityStateChanged 委托 */
	GetMageASC()->AbilityStateChanged.AddLambda(
		[this](const FGameplayTag& AbilityTag, const FGameplayTag& AbilityStateTag, int32 AbilityLevel)
		{
			/** 技能状态变化时，更新按钮状态, 更新技能描述, 并广播 */
			if (SelectedAbility.AbilityTag.MatchesTagExact(AbilityTag))
			{
				// 更新SelectedAbility的StateTag
				SelectedAbility.StateTag = AbilityStateTag;

				//广播
				BroadcastButtonEnabledAndSkillDesc(CurrentSkillPoint);
			}

			/** 根据AbilityTag获取对应的AbilityInfo，广播给WBP_SkillTreeMenu */
			if (AbilityDataAsset)
			{
				FMageAbilityInfo Info = AbilityDataAsset->FindAbilityInfoForTag(AbilityTag);
				Info.StateTag = AbilityStateTag; // 设置StateTag
				AbilityInfoDelegate.Broadcast(Info);
			}
		});

	/** 等级变化->技能点变化 */
	GetMagePlayerState()->OnPlayerSkillPointChanged.AddLambda([this](const int32 NewSkillPoint)
	{
		/** 广播技能点，在WBP_ExperienceBar中绑定 */
		OnSkillPointChangedDelegate.Broadcast(NewSkillPoint);

		/** 技能点变化时，更新按钮状态, 更新技能描述, 并广播*/
		CurrentSkillPoint = NewSkillPoint; // 更新当前技能点

		// 广播
		BroadcastButtonEnabledAndSkillDesc(CurrentSkillPoint);
	});

	/** 绑定SkillEquipped委托 */
	GetMageASC()->SkillEquipped.AddUObject(this, &USkillTreeWidgetController::OnSkillEquippedCallback);
}

void USkillTreeWidgetController::BroadcastButtonEnabledAndSkillDesc(const int32 SkillPoint)
{
	/** 技能点变化时，更新按钮状态, 更新技能描述, 并广播 */
	bool bEnableLearnSkillButton = false;
	bool bEnableEquipSkillButton = false;
	ShouldEnableButton(SelectedAbility.StateTag, SkillPoint, bEnableLearnSkillButton,
					   bEnableEquipSkillButton);
		
	// 获取技能描述
	FString Description;
	FString NextLevelDescription;
	GetMageASC()->GetDescriptionByAbilityTag(SelectedAbility.AbilityTag, Description, NextLevelDescription);

	// 广播 
	OnSkillIconSelectedDelegate.Broadcast(bEnableLearnSkillButton, bEnableEquipSkillButton,Description,NextLevelDescription);
	
}

void USkillTreeWidgetController::SkillIconSelected(const FGameplayTag& AbilityTag)
{
	//停止等待装备的技能播放动画
	if(bWaitingForEquipSelectedSkill)
	{
		const FGameplayTag SelectedAbilityTypeTag = AbilityDataAsset->FindAbilityInfoForTag(SelectedAbility.AbilityTag).TypeTag; //由于还没更新SelectedAbility.AbilityTag，所以这里可以停止上一个点击的Icon动画
		StopWaitingForEquipDelegate.Broadcast(SelectedAbilityTypeTag);
		bWaitingForEquipSelectedSkill = false;
	}
	
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Get();

	const int32 SkillPoint = GetMagePlayerState()->GetSkillPoint();

	const bool bTagValid = AbilityTag.IsValid(); // AbilityTag是否有效
	const bool bTagNone = AbilityTag.MatchesTag(MageGameplayTags.Ability_None); // AbilityTag是否为None

	const FGameplayAbilitySpec* AbilitySpec = GetMageASC()->GetSpecFromAbilityTag(AbilityTag);
	const bool bSpecValid = AbilitySpec != nullptr; // AbilityTag对应的Spec是否有效

	FGameplayTag AbilityStateTag;
	if (!bTagValid || bTagNone || !bSpecValid)
	{
		AbilityStateTag = MageGameplayTags.Ability_State_Locked;
	}
	else
	{
		AbilityStateTag = GetMageASC()->GetStateTagFromSpec(*AbilitySpec);
	}
	
	// 存储选中的技能Tag
	SelectedAbility.AbilityTag = AbilityTag;
	SelectedAbility.StateTag = AbilityStateTag;

	// 广播
	BroadcastButtonEnabledAndSkillDesc(SkillPoint);
}

void USkillTreeWidgetController::LearnSkillButtonPressed()
{
	GetMageASC()->ServerLearnSkill(SelectedAbility.AbilityTag);
}

void USkillTreeWidgetController::SelfUnselect()
{
	//停止等待装备的技能播放动画
	if(bWaitingForEquipSelectedSkill)
	{
		const FGameplayTag SelectedAbilityTypeTag = AbilityDataAsset->FindAbilityInfoForTag(SelectedAbility.AbilityTag).TypeTag;
		StopWaitingForEquipDelegate.Broadcast(SelectedAbilityTypeTag);
		bWaitingForEquipSelectedSkill = false;
	}
	
	// 清空显示信息
	SelectedAbility.AbilityTag = FMageGameplayTags::Get().Ability_None;
	SelectedAbility.StateTag = FMageGameplayTags::Get().Ability_State_Locked;
	OnSkillIconSelectedDelegate.Broadcast(false, false,FString(),FString());
}

void USkillTreeWidgetController::EquipSkillButtonPressed()
{
	// 获取TypeTag并广播给WBP_EquippedIcon
	const FGameplayTag AbilityTypeTag = AbilityDataAsset->FindAbilityInfoForTag(SelectedAbility.AbilityTag).TypeTag;
	WaitForEquipSelectedSkillDelegate.Broadcast(AbilityTypeTag);
	bWaitingForEquipSelectedSkill = true; // 等待装备选中的技能

	// 更新 SelectedSlotInputTag
	const FGameplayTag SelectedStateTag =  GetMageASC()->GetStateTagFromAbilityTag(SelectedAbility.AbilityTag);
	if(SelectedStateTag.MatchesTagExact(FMageGameplayTags::Get().Ability_State_Equipped))
	{
		SelectedSlotInputTag = GetMageASC()->GetInputTagFromAbilityTag(SelectedAbility.AbilityTag);
	}
}

void USkillTreeWidgetController::EquippedSkillIconPressed(const FGameplayTag& SlotInputTag,
	const FGameplayTag& AbilityTypeTag)
{
	if(!bWaitingForEquipSelectedSkill) return;

	// 根据插槽的技能类型检查所选技能，不能将主动技能装备到被动技能插槽（反之也不行）
	const FGameplayTag& SelectedAbilityTypeTag = AbilityDataAsset->FindAbilityInfoForTag(SelectedAbility.AbilityTag).TypeTag;
	if(!SelectedAbilityTypeTag.MatchesTag(AbilityTypeTag)) return;

	// ServerRPC 装备技能逻辑
	GetMageASC()->ServerEquipSkill(SelectedAbility.AbilityTag, SlotInputTag);
}


void USkillTreeWidgetController::OnSkillEquippedCallback(const FGameplayTag& AbilityTag,
	const FGameplayTag& AbilityStateTag, const FGameplayTag& SlotInputTag, const FGameplayTag& PreSlotInputTag)
{
	bWaitingForEquipSelectedSkill = false; 
	
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Get();

	// 如果已经装备了该技能，则清空上一个插槽
	FMageAbilityInfo LastSlotInfo;
	LastSlotInfo.AbilityTag = MageGameplayTags.Ability_None;
	LastSlotInfo.StateTag = MageGameplayTags.Ability_State_Unlocked;
	LastSlotInfo.InputTag = PreSlotInputTag;
	AbilityInfoDelegate.Broadcast(LastSlotInfo);

	//填充新插槽
	FMageAbilityInfo CurrentSlotInfo = AbilityDataAsset->FindAbilityInfoForTag(AbilityTag);
	CurrentSlotInfo.AbilityTag = AbilityTag;
	CurrentSlotInfo.StateTag = AbilityStateTag;
	CurrentSlotInfo.InputTag = SlotInputTag;
	AbilityInfoDelegate.Broadcast(CurrentSlotInfo);

	StopWaitingForEquipDelegate.Broadcast(AbilityDataAsset->FindAbilityInfoForTag(SelectedAbility.AbilityTag).TypeTag);
}

void USkillTreeWidgetController::ShouldEnableButton(const FGameplayTag& AbilityStateTag, int32 SkillPoint,
                                                    bool& bLearnSkillButtonEnabled, bool& bEquipSkillButtonEnabled)
{
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Get();

	bLearnSkillButtonEnabled = false;
	bEquipSkillButtonEnabled = false;

	if (AbilityStateTag.MatchesTagExact(MageGameplayTags.Ability_State_Locked))
	{
		bLearnSkillButtonEnabled = false;
		bEquipSkillButtonEnabled = false;
	}
	else if (AbilityStateTag.MatchesTagExact(MageGameplayTags.Ability_State_Trainable))
	{
		if (SkillPoint > 0) { bLearnSkillButtonEnabled = true; }
		else { bLearnSkillButtonEnabled = false; }

		bEquipSkillButtonEnabled = false;
	}
	else if (AbilityStateTag.MatchesTagExact(MageGameplayTags.Ability_State_Unlocked))
	{
		if (SkillPoint > 0) { bLearnSkillButtonEnabled = true; }
		else { bLearnSkillButtonEnabled = false; }

		bEquipSkillButtonEnabled = true;
	}
	else if (AbilityStateTag.MatchesTagExact(MageGameplayTags.Ability_State_Equipped))
	{
		if (SkillPoint > 0) { bLearnSkillButtonEnabled = true; }
		else { bLearnSkillButtonEnabled = false; }

		bEquipSkillButtonEnabled = true;
	}
}
