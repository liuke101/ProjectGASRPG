#include "GAS/Ability/AbilityTask/AbilityTask_TargetHitUnderCursor.h"

#include "AbilitySystemComponent.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

UAbilityTask_TargetHitUnderCursor* UAbilityTask_TargetHitUnderCursor::WaitTargetHitUnderCursor(UGameplayAbility* OwningAbility)
{
	UAbilityTask_TargetHitUnderCursor* MyObj = NewAbilityTask<UAbilityTask_TargetHitUnderCursor>(OwningAbility);
	return MyObj;
}

void UAbilityTask_TargetHitUnderCursor::Activate()
{
	SendCursorHitData();
}

void UAbilityTask_TargetHitUnderCursor::SendCursorHitData()
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

