#include "UI/WidgetController/AttributeMenuWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/Data/AttributeDataAsset.h"
#include "Player/MagePlayerState.h"

void UAttributeMenuWidgetController::BroadcastInitialValue()
{
	checkf(AttributeDataAsset, TEXT("AttributeInfo为空, 请在BP_AttributeMenuWidgetController中设置"));

	for(auto& Pair : GetMageAttributeSet()->AttributeTag_To_GetAttributeFuncPtr)
	{
		BroadcastAttributeInfo(Pair.Key, Pair.Value().GetNumericValue(GetMageAttributeSet())); //Pair.Value是函数名，要加()进行调用返回FGameplayAttribute
	}
#pragma region 旧代码等价实现
	// FMageAttributeInfo Info_Health = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Vital_Health, true);
	// Info_Health.AttributeValue = MageAttributeSet->GetHealth();
	// AttributeInfoDelegate.Broadcast(Info_Health);
	// 继续列举所有属性 ...
#pragma endregion

	/** 初始化LevelData */
	OnLevelChangedDelegate.Broadcast(GetMagePlayerState()->GetCharacterLevel()); 
	OnAttributePointChangedDelegate.Broadcast(GetMagePlayerState()->GetAttributePoint()); 
	OnSkillPointChangedDelegate.Broadcast(GetMagePlayerState()->GetSkillPoint());  
}

void UAttributeMenuWidgetController::BroadcastAttributeInfo(const FGameplayTag& GameplayTag, const float AttributeCurrentValue)
{
	FMageAttributeInfo Info = AttributeDataAsset->FindAttributeInfoForTag(GameplayTag, true);
	Info.AttributeValue = AttributeCurrentValue;
	AttributeInfoDelegate.Broadcast(Info);
}

void UAttributeMenuWidgetController::BindCallbacks()
{
	/** 绑定属性变化回调，接收属性变化 */
	for(auto& Pair : GetMageAttributeSet()->AttributeTag_To_GetAttributeFuncPtr)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Value()).AddLambda([this,Pair](const FOnAttributeChangeData& Data)
		{
			BroadcastAttributeInfo(Pair.Key, Data.NewValue);
		});
	}

	/** 绑定 PlayerState 数据变化回调 */
	/** 等级变化 */
	GetMagePlayerState()->OnPlayerLevelChanged.AddLambda([this](const int32 NewLevel)
	{
		OnLevelChangedDelegate.Broadcast(NewLevel); // 广播等级 
	});
	
	/** 属性点变化 */
	GetMagePlayerState()->OnPlayerAttributePointChanged.AddLambda([this](const int32 NewAttributePoint)
	{
		OnAttributePointChangedDelegate.Broadcast(NewAttributePoint); // 广播属性点
	});
	
	/** 技能点变化 */
	GetMagePlayerState()->OnPlayerSkillPointChanged.AddLambda([this](const int32 NewSkillPoint)
	{
		OnSkillPointChangedDelegate.Broadcast(NewSkillPoint);// 广播技能点 
	});
}

void UAttributeMenuWidgetController::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	GetMageASC()->UpgradeAttribute(AttributeTag);
}
