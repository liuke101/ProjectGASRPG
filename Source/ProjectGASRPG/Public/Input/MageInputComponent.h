#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputConfigDataAsset.h"
#include "MageInputComponent.generated.h"


class UInputConfigDataAsset;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UMageInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	/** 绑定 InputConfigDataAsset 中所有的InputAction(每种ETriggerEvent对应一个回调函数)
	 * - 对于默认的Input Triggers设置：
	 *		- 按下: 触发 Started 事件
	 *		- 保持: 不断触发 Triggered 事件
	 *		- 松开: 触发 Completed 事件
	 */
	template <typename UserClass, typename StartedFuncType, typename OngoingFuncType, typename TriggeredFuncType, typename CanceledFuncType, typename CompletedFuncType>
	void BindAbilityInputActions(const UInputConfigDataAsset* InputConfigDataAsset, UserClass* Object, StartedFuncType StartedFunc,OngoingFuncType OngoingFunc, TriggeredFuncType TriggeredFunc,CanceledFuncType CanceledFunc, CompletedFuncType CompletedFunc);
};

template <typename UserClass, typename StartedFuncType, typename OngoingFuncType, typename TriggeredFuncType, typename
	CanceledFuncType, typename CompletedFuncType>
void UMageInputComponent::BindAbilityInputActions(const UInputConfigDataAsset* InputConfigDataAsset, UserClass* Object,
	StartedFuncType StartedFunc, OngoingFuncType OngoingFunc, TriggeredFuncType TriggeredFunc,
	CanceledFuncType CanceledFunc, CompletedFuncType CompletedFunc)
{
	checkf(InputConfigDataAsset, TEXT("InputConfigDataAsset 为空, 请在 MagePlayerController 中设置"));

	// 绑定所有的InputAction(每种ETriggerEvent对应一个回调函数)
	for (const FMageInputAction& Action : InputConfigDataAsset->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (StartedFunc)
			{
				/* 最后一个参数是绑定的Tag, 作为回调函数的参数 */
				BindAction(Action.InputAction, ETriggerEvent::Started, Object, StartedFunc, Action.InputTag);
			}

			if(OngoingFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Ongoing, Object, OngoingFunc, Action.InputTag);
			}

			if (TriggeredFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, TriggeredFunc, Action.InputTag);
			}

			if(CanceledFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Canceled, Object, CanceledFunc, Action.InputTag);
			}

			if (CompletedFunc)
			{
				BindAction(Action.InputAction, ETriggerEvent::Completed, Object, CompletedFunc, Action.InputTag);
			}
		}
	}
}
