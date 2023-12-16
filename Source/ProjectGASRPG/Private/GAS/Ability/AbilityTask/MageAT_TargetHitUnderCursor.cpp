#include "GAS/Ability/AbilityTask/MageAT_TargetHitUnderCursor.h"

#include "AbilitySystemComponent.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

UMageAT_TargetHitUnderCursor* UMageAT_TargetHitUnderCursor::WaitTargetHitUnderCursor(UGameplayAbility* OwningAbility)
{
	UMageAT_TargetHitUnderCursor* MyObj = NewAbilityTask<UMageAT_TargetHitUnderCursor>(OwningAbility);
	return MyObj;
}

void UMageAT_TargetHitUnderCursor::Activate()
{
	SendCursorHitData();
}

void UMageAT_TargetHitUnderCursor::SendCursorHitData()
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
		//可以Add多个TargetData,通过index访问

		/** 广播 */
		if(ShouldBroadcastAbilityTaskDelegates())
		{
			ValidData.Broadcast(TargetDataHandle);
		}
	}
}

