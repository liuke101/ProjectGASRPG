#include "UI/WidgetController/OverlayWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/Data/AbilityDataAsset.h"
#include "GAS/Data/LevelDataAsset.h"
#include "Player/MagePlayerState.h"

void UOverlayWidgetController::BroadcastInitialValue()
{
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);

	OnHealthChanged.Broadcast(MageAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(MageAttributeSet->GetMaxHealth());
	OnManaChanged.Broadcast(MageAttributeSet->GetMana());
	OnMaxManaChanged.Broadcast(MageAttributeSet->GetMaxMana());
	OnVitalityChanged.Broadcast(MageAttributeSet->GetVitality());
	OnMaxVitalityChanged.Broadcast(MageAttributeSet->GetMaxVitality());

	OnExpChangedCallback(0); //初始化经验条
}

void UOverlayWidgetController::BindCallbacks()
{
	/** 绑定 PlayerState 数据变化回调 */
	if(AMagePlayerState* MagePlayerState = Cast<AMagePlayerState>(PlayerState))
	{
		/** 经验值变化 */
		MagePlayerState->OnPlayerExpChanged.AddUObject(this, &UOverlayWidgetController::OnExpChangedCallback);
	}
	
	/** 绑定ASC属性变化回调，接收属性变化 */
	if(const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnManaChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxManaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChanged.Broadcast(Data.NewValue);
		});

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetVitalityAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnVitalityChanged.Broadcast(Data.NewValue);
		});

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxVitalityAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxVitalityChanged.Broadcast(Data.NewValue);
		});
	}
	
	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(AbilitySystemComponent))
	{
		/** 
		 * 绑定 AbilitiesGiven 委托
		 * - 如果已经授予了Ability，可以直接执行回调
		 * - 否则绑定委托回调，等待GiveCharacterAbilities()执行
		 */
		if(MageASC->bCharacterAbilitiesGiven)
		{
			OnInitializeStartupAbilities(MageASC);
		}
		else 
		{
			MageASC->AbilitiesGiven.AddUObject(this, &UOverlayWidgetController::OnInitializeStartupAbilities);
		}
		
		/** 绑定 EffectAssetTags 回调，接收 GameplayTagContainer */
		MageASC->EffectAssetTags.AddLambda([this](const FGameplayTagContainer& AssetTags)
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

}

void UOverlayWidgetController::OnInitializeStartupAbilities(UMageAbilitySystemComponent* MageASC)
{
	if(!MageASC->bCharacterAbilitiesGiven) return;
	
	FForEachAbilityDelegate AbilityDelegate;
	// 绑定AbilityDelegate委托
	AbilityDelegate.BindLambda([this, MageASC](const FGameplayAbilitySpec& AbilitySpec)
	{
		// 根据AbilitySpec获取AbilityTag, 然后根据AbilityTag获取AbilityInfo
		FMageAbilityInfo Info = AbilityDataAsset->FindAbilityInfoForTag(MageASC->GetAbilityTagFromSpec(AbilitySpec));
		Info.InputTag = MageASC->GetInputTagFromSpec(AbilitySpec);

		// 注意这是另一个委托，负责将 AbilityInfo 广播给 UserWidget
		AbilityInfoDelegate.Broadcast(Info);
	});

	/**
	 * 执行AbilityDelegate委托, 结合委托绑定的Lambda，同时也将AbilityInfoDelegate委托广播。
	 * 这样所有被授予的Ability都会将信息被广播给OverlayUserWidget
	 */
	MageASC->ForEachAbility(AbilityDelegate);
}

void UOverlayWidgetController::OnExpChangedCallback(int32 NewExp) const
{
	if(const AMagePlayerState* MagePlayerState = Cast<AMagePlayerState>(PlayerState))
	{
		ULevelDataAsset* LevelDataAsset = MagePlayerState->LevelDataAsset;
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
}




