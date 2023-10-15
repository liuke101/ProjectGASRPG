// 

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "MageInputConfig.h"
#include "MageInputComponent.generated.h"


class UMageInputConfig;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UMageInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	/* 绑定InputAction，同时支持 【按下】、【释放】、【持续】 三种回调函数 */
	template <typename UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityInputActions(const UMageInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,HeldFuncType HoldFunc);
};

template <typename UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UMageInputComponent::BindAbilityInputActions(const UMageInputConfig* InputConfig, UserClass* Object,
	PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, HeldFuncType HoldFunc)
{
	check(InputConfig);

	// 绑定所有的InputAction
	for (const auto& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if(PressedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag); //最后一个参数是绑定的Tag
			}
			
			if(ReleasedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag); //最后一个参数是绑定的Tag
			}
			
			if(HoldFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, HoldFunc, Action.InputTag); 
			}
			
		}
	}
}





