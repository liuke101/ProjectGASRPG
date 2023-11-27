// 


#include "GAS/Ability/BeamGA.h"

#include "InputState.h"
#include "Interface/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

void UBeamGA::StoreMouseDataInfo(const FHitResult& HitResult)
{
	if(HitResult.bBlockingHit)
	{
		MouseHitLocation = HitResult.ImpactPoint;
		MouseHitActor = HitResult.GetActor();
	}
	else
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UBeamGA::TraceFirstTarget(const FVector& TargetLocation)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if( AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		if(USkeletalMeshComponent* Weapon = ICombatInterface::Execute_GetWeapon(AvatarActor))
		{
			const FVector SocketLocation = Weapon->GetSocketLocation("TipSocket");
			FHitResult HitResult;

			// 闪电路径使用SphereTrace模拟（圆柱形）
			UKismetSystemLibrary::SphereTraceSingle(AvatarActor,SocketLocation,TargetLocation,10.f,TraceTypeQuery1,false,TArray{AvatarActor},EDrawDebugTrace::None,HitResult,true);

			if(HitResult.bBlockingHit)
			{
				MouseHitLocation = HitResult.ImpactPoint;
				MouseHitActor = HitResult.GetActor();
			}
		}
	}
}

