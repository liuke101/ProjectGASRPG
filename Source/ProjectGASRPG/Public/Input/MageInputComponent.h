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
	template <typename UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
	void BindAbilityInputActions(const UMageInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, HeldFuncType HoldFunc, ReleasedFuncType ReleasedFunc);
};

template <typename UserClass, typename PressedFuncType, typename ReleasedFuncType, typename HeldFuncType>
void UMageInputComponent::BindAbilityInputActions(const UMageInputConfig* InputConfig, UserClass* Object,
	PressedFuncType PressedFunc, HeldFuncType HoldFunc, ReleasedFuncType ReleasedFunc)
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
			if(HoldFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, HoldFunc, Action.InputTag); 
			}
			if(ReleasedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag); //最后一个参数是绑定的Tag
			}
		}
	}
}



