﻿#include "GAS/Ability/BeamGA.h"
#include "GAS/MageAbilitySystemLibrary.h"
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

void UBeamGA::StoreAdditionalTarget(TArray<AActor*>& OutAdditionalTargets, const int32 TargetNum, const float Radius)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if( AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		// 获取半径内的所有活着的Player
		// 忽略AvatarActor和MouseHitActor
		TArray<AActor*> OverlappingActors;
		UMageAbilitySystemLibrary::GetLivePlayerWithInRadius(GetAvatarActorFromActorInfo(),OverlappingActors,
		TArray<AActor*>{AvatarActor,MouseHitActor}, MouseHitActor->GetActorLocation(),Radius);

		
		// 获取距离最近的Actor
		//TargetNum = FMath::Min(GetAbilityLevel(), MaxTargetNum);
		UMageAbilitySystemLibrary::GetClosestActors(OverlappingActors,OutAdditionalTargets,MouseHitActor->GetActorLocation(), TargetNum);
	}
}

