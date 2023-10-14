// 


#include "UI/WidgetController/AttributeMenuWidgetController.h"

#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Data/AttributeInfo.h"

void UAttributeMenuWidgetController::BroadcastInitialValue()
{
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);
	checkf(AttributeInfo, TEXT("AttributeInfo为空, 请在BP_AttributeMenuWidgetController中设置"));

	for(auto& Pair : MageAttributeSet->TagsToAttributes)
	{
		FMageAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(Pair.Key, true);
		Info.AttributeValue = Pair.Value().GetNumericValue(MageAttributeSet); //Pair.Value是函数名，要加()进行调用返回FGameplayAttribute
		AttributeInfoDelegate.Broadcast(Info);
	}
#pragma region 旧代码等价实现
	// FMageAttributeInfo Info_Health = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Vital_Health, true);
	// Info_Health.AttributeValue = MageAttributeSet->GetHealth();
	// AttributeInfoDelegate.Broadcast(Info_Health);
	// 继续列举所有属性 ...
#pragma endregion
}

void UAttributeMenuWidgetController::BindCallbacks()
{
	
}
