#include "GAS/Ability/AbilityTask/TargetDataUnderMouse.h"

#include "AbilitySystemComponent.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

UTargetDataUnderMouse* UTargetDataUnderMouse::CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility)
{
	UTargetDataUnderMouse* MyObj = NewAbilityTask<UTargetDataUnderMouse>(OwningAbility);
	return MyObj;
}

void UTargetDataUnderMouse::Activate()
{
	SendMouseCursorData();
}

void UTargetDataUnderMouse::SendMouseCursorData()
{
	if(const APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get())
	{
		/**
		 * 打包命中结果数据
		 * - 注意这里设置了自定义的碰撞通道 ECC_Target, 默认为BlockAll
		 */
		FHitResult CursorHit;
		PlayerController->GetHitResultUnderCursor(ECC_Target, false, CursorHit);

		/** 创建TargetData, 类型为SingleTargetHit */
		FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit();
		TargetData->HitResult = CursorHit;

		/** 打包TargetData */
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(TargetData);

		/** 广播 */
		if(ShouldBroadcastAbilityTaskDelegates())
		{
			ValidData.Broadcast(TargetDataHandle);
		}
	}
}

