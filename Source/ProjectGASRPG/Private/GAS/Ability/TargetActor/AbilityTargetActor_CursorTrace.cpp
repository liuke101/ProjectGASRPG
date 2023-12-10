#include "GAS/Ability/TargetActor/AbilityTargetActor_CursorTrace.h"

#include "Abilities/GameplayAbility.h"
#include "Interface/InteractionInterface.h"
#include "ProjectGASRPG/ProjectGASRPG.h"


void AAbilityTargetActor_CursorTrace::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
}

FHitResult AAbilityTargetActor_CursorTrace::PerformTrace(AActor* InSourceActor)
{
	const APlayerController* PlayerController = OwningAbility->GetCurrentActorInfo()->PlayerController.Get();
	check(PlayerController);
	/**
	 * 打包命中结果数据
	 * - 注意这里设置了自定义的碰撞通道 ECC_Target, 默认为BlockAll
	 */
	FHitResult CursorHit;
	PlayerController->GetHitResultUnderCursor(ECC_Target, false, CursorHit);
	
	if (AActor* Actor = CursorHit.GetActor())
	{
		/* 条件判断 */
		bLastTraceWasGood = Actor->Implements<UInteractionInterface>();
		/* 依据检测结果，通过蓝图事件设置不同的演示效果 */

		/** Reticle */
		if(bLastTraceWasGood)
		{
			if (AGameplayAbilityWorldReticle* LocalReticleActor = ReticleActor.Get())
			{
				LocalReticleActor->SetIsTargetValid(bLastTraceWasGood);
				LocalReticleActor->SetActorLocation(CursorHit.Location);
			}
		}
	}
	
	return CursorHit;
}
	
bool AAbilityTargetActor_CursorTrace::IsConfirmTargetingAllowed()
{
	return bLastTraceWasGood;
}
