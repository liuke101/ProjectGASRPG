#include "GAS/AsyncTask/ListenForCooldownChange.h"
#include "AbilitySystemComponent.h"

UListenForCooldownChange* UListenForCooldownChange::ListenForCooldownChange(UAbilitySystemComponent* InASC, const FGameplayTag& InCooldownTag)
{
	UListenForCooldownChange* ListenForCooldownChange = NewObject<UListenForCooldownChange>();
	ListenForCooldownChange->ASC = InASC;
	ListenForCooldownChange->CooldownTag = InCooldownTag;

	if(!IsValid(InASC) || !InCooldownTag.IsValid())
	{
		ListenForCooldownChange->EndTask();
		return nullptr;
	}
	
	/** 当cooldown GE 被添加或移除时，触发回调 */
	InASC->RegisterGameplayTagEvent(InCooldownTag, EGameplayTagEventType::NewOrRemoved).AddUObject(ListenForCooldownChange, &UListenForCooldownChange::CooldownTagChangedCallback);

	/** 当cooldown GE 应用到自身时，触发回调 */
	InASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(ListenForCooldownChange, &UListenForCooldownChange::OnActiveEffectAddedToSelfCallback);

	return ListenForCooldownChange;
}

void UListenForCooldownChange::EndTask()
{
	if(!IsValid(ASC)) return;
	
	ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);

	SetReadyToDestroy();
	MarkAsGarbage();
	
}

void UListenForCooldownChange::CooldownTagChangedCallback(const FGameplayTag GameplayTag, int32 NewCount) const
{
	if(NewCount==0)
	{
		CooldownEnd.Broadcast(0.0f);
	}
}

void UListenForCooldownChange::OnActiveEffectAddedToSelfCallback(UAbilitySystemComponent* InASC,
	const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle EffectHandle) const
{
	FGameplayTagContainer AssetTags;
	EffectSpec.GetAllAssetTags(AssetTags);
	
	FGameplayTagContainer GrantedTags;
	EffectSpec.GetAllGrantedTags(GrantedTags);

	/** 如果GE的AssetTags或GrantedTags中有CooldownTag, 则获取剩余时间，并由CooldownStart广播 */
	if(AssetTags.HasTagExact(CooldownTag) || GrantedTags.HasTagExact(CooldownTag))
	{
		/**
		 * 获取所有匹配CooldownTag的GE的剩余时间,通常CooldownTag标签是特有的，所以数组只有一个元素, 返回第一个元素即可
		 * - 为了防止有多个元素，遍历数组，取最大剩余时间
		 */
		FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTag.GetSingleTagContainer()); 
		TArray<float> ActiveEffectsTimeRemaining = InASC->GetActiveEffectsTimeRemaining(Query);
		if(!ActiveEffectsTimeRemaining.IsEmpty())
		{
			float TimeRemaining = ActiveEffectsTimeRemaining[0];
			for(int32 i=1; i<ActiveEffectsTimeRemaining.Num(); i++)
			{
				if(ActiveEffectsTimeRemaining[i]>TimeRemaining)
				{
					TimeRemaining = ActiveEffectsTimeRemaining[i];
				}
			}
			
			CooldownStart.Broadcast(TimeRemaining);
		}
		
	}

	
	
}
