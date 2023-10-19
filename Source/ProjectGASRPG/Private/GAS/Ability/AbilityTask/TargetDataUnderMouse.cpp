﻿#include "GAS/Ability/AbilityTask/TargetDataUnderMouse.h"

#include "AbilitySystemComponent.h"

UTargetDataUnderMouse* UTargetDataUnderMouse::CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility)
{
	UTargetDataUnderMouse* MyObj = NewAbilityTask<UTargetDataUnderMouse>(OwningAbility);
	return MyObj;
}

void UTargetDataUnderMouse::Activate()
{
	if(IsLocallyControlled())  
	{
		SendMouseCursorData();
	}
	else
	{
		const FGameplayAbilitySpecHandle SPecHandle = GetAbilitySpecHandle();
		const FPredictionKey PredictionKey = GetActivationPredictionKey();
		
		/**
		 * 服务器激活时绑定 TargetDataSet 委托
		 *
		 * 这样当TargetData到达服务器时，TargetDataSet 委托将被广播，因为服务器已经提前绑定了该委托，因此可以接收到数据执行回调
		 */
		AbilitySystemComponent.Get()->AbilityTargetDataSetDelegate(SPecHandle, PredictionKey).AddUObject(this, &UTargetDataUnderMouse::OnTargetDataReplicatedCallback);

		/**
		 * 1. 如果服务器接收到TargetData，就广播TargetDataSet委托
		 * 2. 如果没有调用委托，说明服务器没有数据，就等待数据
		 */
		const bool bCalledDelegate = AbilitySystemComponent.Get()->CallReplicatedTargetDataDelegatesIfSet(SPecHandle, PredictionKey);
		if(!bCalledDelegate) 
		{
			SetWaitingOnRemotePlayerData(); // 等待远程玩家数据
		}
	}
}

void UTargetDataUnderMouse::SendMouseCursorData()
{
	
	if(const APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get())
	{
		/** 范围预测窗口,指定该范围内的行为都是可以预测的 */
		FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get());

		/** 打包命中结果数据 */
		FHitResult CursorHit;
		PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
		
		FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit();
		TargetData->HitResult = CursorHit;

		/** 打包TargetData */
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(TargetData);

		/** 复制TargetDataHandle（内含TargetData）到服务器 */
		AbilitySystemComponent->ServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(),TargetDataHandle, FGameplayTag(), AbilitySystemComponent->ScopedPredictionKey);

		/** 广播 */
		if(ShouldBroadcastAbilityTaskDelegates())
		{
			ValidData.Broadcast(TargetDataHandle);
		}
	}

	
	
	
}

void UTargetDataUnderMouse::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& TargetDataHandle,
	FGameplayTag ActivationTag)
{
	/** 客户端清空已存储的TargetData */
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());

	/** 广播 */
	if(ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(TargetDataHandle);
	}
}
