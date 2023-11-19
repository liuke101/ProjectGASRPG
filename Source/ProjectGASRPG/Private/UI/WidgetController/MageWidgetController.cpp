#include "UI/WidgetController/MageWidgetController.h"

#include "GAS/MageAttributeSet.h"
#include "Player/MagePlayerController.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/Data/AbilityDataAsset.h"
#include "Player/MagePlayerState.h"


void UMageWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	AttributeSet = WCParams.AttributeSet;
}

void UMageWidgetController::BroadcastInitialValue()
{
	//...
}

void UMageWidgetController::BindCallbacks()
{
	//...
}

void UMageWidgetController::BroadcastAbilityInfo()
{
	if(!GetMageASC()->bCharacterAbilitiesGiven) return;
	
	FForEachAbilityDelegate AbilityDelegate;
	
	// 绑定AbilityDelegate委托，相当于遍历了所有授予的Ability对一个的AbilitySpec
	AbilityDelegate.BindLambda([this](const FGameplayAbilitySpec& AbilitySpec)
	{
		// 根据AbilitySpec获取AbilityTag, 然后根据AbilityTag获取AbilityInfo
		FMageAbilityInfo Info = AbilityDataAsset->FindAbilityInfoForTag(GetMageASC()->GetAbilityTagFromSpec(AbilitySpec));
		Info.InputTag = GetMageASC()->GetInputTagFromSpec(AbilitySpec);

		// 注意这是另一个委托，负责将 AbilityInfo 广播给 UserWidget
		AbilityInfoDelegate.Broadcast(Info);
	});

	/**
	 * 执行AbilityDelegate委托, 结合委托绑定的Lambda，同时也将AbilityInfoDelegate委托广播。
	 * 这样所有被授予的Ability都会将信息被广播给OverlayUserWidget
	 */
	GetMageASC()->ForEachAbility(AbilityDelegate);
}

AMagePlayerController* UMageWidgetController::GetMagePlayerController()
{
	if(MagePlayerController == nullptr)
	{
		MagePlayerController = Cast<AMagePlayerController>(PlayerController);
	}
	return MagePlayerController;
}

AMagePlayerState* UMageWidgetController::GetMagePlayerState()
{
	if(MagePlayerState == nullptr)
	{
		MagePlayerState = Cast<AMagePlayerState>(PlayerState);
	}
	return MagePlayerState;
}

UMageAbilitySystemComponent* UMageWidgetController::GetMageASC()
{
	if(MageAbilitySystemComponent == nullptr)
	{
		MageAbilitySystemComponent = Cast<UMageAbilitySystemComponent>(AbilitySystemComponent);
	}
	return MageAbilitySystemComponent;
}

UMageAttributeSet* UMageWidgetController::GetMageAttributeSet()
{
	if(MageAttributeSet == nullptr)
	{
		MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);
	}
	return MageAttributeSet;
}
