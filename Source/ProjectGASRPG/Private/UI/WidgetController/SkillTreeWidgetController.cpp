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
			/** 技能状态变化时，更新按钮状态 */
			if (SelectedAbility.AbilityTag.MatchesTagExact(AbilityTag))
			{
				// 更新SelectedAbility的StateTag
				SelectedAbility.StateTag = AbilityStateTag;

				bool bEnableLearnSkillButton = false;
				bool bEnableEquipSkillButton = false;
				ShouldEnableButton(SelectedAbility.StateTag, CurrentSkillPoint, bEnableLearnSkillButton,
				                   bEnableEquipSkillButton);
				// 广播 
				CheckButtonEnableOnSkillIconSelected.Broadcast(bEnableLearnSkillButton, bEnableEquipSkillButton);
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

		/** 技能点变化时，更新按钮状态 */
		CurrentSkillPoint = NewSkillPoint; // 更新当前技能点
		bool bEnableLearnSkillButton = false;
		bool bEnableEquipSkillButton = false;
		ShouldEnableButton(SelectedAbility.StateTag, CurrentSkillPoint, bEnableLearnSkillButton,
		                   bEnableEquipSkillButton);
		// 广播 
		CheckButtonEnableOnSkillIconSelected.Broadcast(bEnableLearnSkillButton, bEnableEquipSkillButton);
	});
}

void USkillTreeWidgetController::SkillIconSelected(const FGameplayTag& AbilityTag)
{
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

	bool bEnableLearnSkillButton = false;
	bool bEnableEquipSkillButton = false;
	ShouldEnableButton(AbilityStateTag, SkillPoint, bEnableLearnSkillButton, bEnableEquipSkillButton);

	// 广播 
	CheckButtonEnableOnSkillIconSelected.Broadcast(bEnableLearnSkillButton, bEnableEquipSkillButton);
}

void USkillTreeWidgetController::LearnSkillButtonPressed()
{
	GetMageASC()->ServerLearnSkill(SelectedAbility.AbilityTag);
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
