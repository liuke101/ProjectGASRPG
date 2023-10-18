#include "GAS/Ability/AbilityTask/TargetDataUnderMouse.h"

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
		//服务器监听TargetData
	}
	
	// if(const APlayerController* PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get())
	// {
	// 	FHitResult CursorHit;
	// 	PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	// 	ValidData.Broadcast(CursorHit.ImpactPoint);
	// }
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
