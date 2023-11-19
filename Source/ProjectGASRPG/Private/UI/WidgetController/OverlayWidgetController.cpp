#include "UI/WidgetController/OverlayWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/Data/AbilityDataAsset.h"
#include "GAS/Data/LevelDataAsset.h"
#include "Player/MagePlayerState.h"

void UOverlayWidgetController::BroadcastInitialValue()
{
	/** 初始化Attribute */
	OnHealthChanged.Broadcast(GetMageAttributeSet()->GetHealth());
	OnMaxHealthChanged.Broadcast(GetMageAttributeSet()->GetMaxHealth());
	OnManaChanged.Broadcast(GetMageAttributeSet()->GetMana());
	OnMaxManaChanged.Broadcast(GetMageAttributeSet()->GetMaxMana());
	OnVitalityChanged.Broadcast(GetMageAttributeSet()->GetVitality());
	OnMaxVitalityChanged.Broadcast(GetMageAttributeSet()->GetMaxVitality());

	/** 初始化LevelData */
	OnExpChangedCallback(GetMagePlayerState()->GetExp()); 
	OnLevelChangedDelegate.Broadcast(GetMagePlayerState()->GetCharacterLevel()); 
	OnAttributePointChangedDelegate.Broadcast(GetMagePlayerState()->GetAttributePoint()); 
	OnSkillPointChangedDelegate.Broadcast(GetMagePlayerState()->GetSkillPoint());  
}

void UOverlayWidgetController::BindCallbacks()
{
	/** 绑定 PlayerState 数据变化回调 */
	/** 经验值变化 */
	GetMagePlayerState()->OnPlayerExpChanged.AddUObject(this, &UOverlayWidgetController::OnExpChangedCallback);

	/** 等级变化 */
	GetMagePlayerState()->OnPlayerLevelChanged.AddLambda([this](const int32 NewLevel)
	{
		OnLevelChangedDelegate.Broadcast(NewLevel); //  广播等级，在WBP_ExperienceBar中绑定 
	});
		
	
	/** 绑定ASC属性变化回调，接收属性变化 */
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetMageAttributeSet()->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnHealthChanged.Broadcast(Data.NewValue);
	});

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetMageAttributeSet()->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnMaxHealthChanged.Broadcast(Data.NewValue);
	});

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetMageAttributeSet()->GetManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnManaChanged.Broadcast(Data.NewValue);
	});

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetMageAttributeSet()->GetMaxManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnMaxManaChanged.Broadcast(Data.NewValue);
	});

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetMageAttributeSet()->GetVitalityAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnVitalityChanged.Broadcast(Data.NewValue);
	});

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetMageAttributeSet()->GetMaxVitalityAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
	{
		OnMaxVitalityChanged.Broadcast(Data.NewValue);
	});
	
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
		GetMageASC()->AbilitiesGiven.AddUObject(this, &UOverlayWidgetController::BroadcastAbilityInfo);
	}
	
	/** 绑定 EffectAssetTags 回调，接收 GameplayTagContainer */
	GetMageASC()->EffectAssetTags.AddLambda([this](const FGameplayTagContainer& AssetTags)
	{
		for (auto& Tag : AssetTags)
		{
			/**
			 * "A.B".MatchesTag("A") 返回 True, "A".MatchesTag("A.B") 返回 False
			 * 若TagtoCheck无效总是返回 False
			 *
			 * 例如，Tag = "Message.HealthPotion"，则 Tag.MatchesTag("Message") 返回 True
			 */
			FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
			if (Tag.MatchesTag(MessageTag))
			{
				if (const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag))
				{
					MessageWidgetRowDelegate.Broadcast(*Row); //广播数据表行, 在 OverlayUserWidget 蓝图中绑定该委托
				}
			}
		}
	});
}

void UOverlayWidgetController::OnExpChangedCallback(const int32 NewExp)
{
	ULevelDataAsset* LevelDataAsset = GetMagePlayerState()->LevelDataAsset;
	checkf(LevelDataAsset,TEXT("LevelDataAsset为空，请在BP_MagePlayerState中设置"));

	const int32 Level = LevelDataAsset->FindLevelForExp(NewExp);
	const int32 MaxLevel = LevelDataAsset->LevelUpInfos.Num(); //最大等级由LevelUpInfos的元素数量决定

	if(Level <= MaxLevel && Level > 0)
	{
		/** 计算经验值，注意，DataAsset中表示的是从0开始不断累积的经验值，而在经验条UI中的显示则是每次升级从0开始计算经验，所以需要做一个转变计算 */
		const int32 LevelUpRequirement = LevelDataAsset->LevelUpInfos[Level].LevelUpRequirement;
		const int32 PreviousLevelUpRequirement = LevelDataAsset->LevelUpInfos[Level - 1].LevelUpRequirement; //上一级的升级需求
		const int32 DeltaLevelUpRequirement = LevelUpRequirement - PreviousLevelUpRequirement; //当前等级的升级需求经验量
		const int32 ExpForThisLevel = NewExp - PreviousLevelUpRequirement; //当前的经验值
		
		/** 广播当前经验值和所需经验值，在WBP_ExperienceBar中绑定 */
		OnExpChangedDelegate.Broadcast(ExpForThisLevel,DeltaLevelUpRequirement);
	}
}




