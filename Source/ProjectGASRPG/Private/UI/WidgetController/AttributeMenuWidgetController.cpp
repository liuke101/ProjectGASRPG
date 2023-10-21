#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/Data/AttributeDataAsset.h"

void UAttributeMenuWidgetController::BroadcastInitialValue()
{
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);
	checkf(AttributeInfo, TEXT("AttributeInfo为空, 请在BP_AttributeMenuWidgetController中设置"));

	for(auto& Pair : MageAttributeSet->TagsToAttributes)
	{
		BroadcastAttributeInfo(Pair.Key, Pair.Value().GetNumericValue(MageAttributeSet)); //Pair.Value是函数名，要加()进行调用返回FGameplayAttribute
	}
#pragma region 旧代码等价实现
	// FMageAttributeInfo Info_Health = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Vital_Health, true);
	// Info_Health.AttributeValue = MageAttributeSet->GetHealth();
	// AttributeInfoDelegate.Broadcast(Info_Health);
	// 继续列举所有属性 ...
#pragma endregion
}

void UAttributeMenuWidgetController::BroadcastAttributeInfo(const FGameplayTag& GameplayTag, const float AttributeCurrentValue)
{
	FMageAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(GameplayTag, true);
	Info.AttributeValue = AttributeCurrentValue;
	AttributeInfoDelegate.Broadcast(Info);
}

void UAttributeMenuWidgetController::BindCallbacks()
{
	/** 绑定属性变化回调，接收属性变化 */
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);

	for(auto& Pair : MageAttributeSet->TagsToAttributes)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Value()).AddLambda([this,Pair](const FOnAttributeChangeData& Data)
		{
			BroadcastAttributeInfo(Pair.Key, Data.NewValue);
		});
	}
}
