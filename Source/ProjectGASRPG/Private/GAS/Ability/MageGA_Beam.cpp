#include "GAS/Ability/MageGA_Beam.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Interface/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

void UMageGA_Beam::TraceFirstTarget(const FVector& TargetLocation)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if(AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		if(USkeletalMeshComponent* Weapon = ICombatInterface::Execute_GetWeapon(AvatarActor))
		{
			const FVector SocketLocation = Weapon->GetSocketLocation("TipSocket");
			FHitResult HitResult;

			// 闪电路径使用SphereTrace模拟（圆柱形）
			UKismetSystemLibrary::SphereTraceSingle(AvatarActor,SocketLocation,TargetLocation,10.f,TraceTypeQuery1,false,TArray{AvatarActor},EDrawDebugTrace::None,HitResult,true);

			if(HitResult.bBlockingHit)
			{
				TargetingActorLocation = HitResult.ImpactPoint;
				TargetingActor = HitResult.GetActor();
			}
		}
	}

	// MouseHitActor绑定OnDeath委托，当MouseHitActor死亡时，调用回调,强制关闭GC
	if(ICombatInterface* CombatInterface = Cast<ICombatInterface>(TargetingActor))
	{
		if(!CombatInterface->GetOnDeathDelegate().IsAlreadyBound(this,&UMageGA_Beam::OnTargetDiedCallback))
		{
			CombatInterface->GetOnDeathDelegate().AddDynamic(this,&UMageGA_Beam::OnTargetDiedCallback);
		}
	}
}

void UMageGA_Beam::StoreAdditionalTarget(TArray<AActor*>& OutAdditionalTargets, const int32 TargetNum, const float Radius)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if( AvatarActor && AvatarActor->Implements<UCombatInterface>())
	{
		// 获取半径内的所有活着的Player
		// 忽略AvatarActor和MouseHitActor
		TArray<AActor*> OverlappingActors;
		// UMageAbilitySystemLibrary::GetLivePlayerWithInRadius(GetAvatarActorFromActorInfo(),OverlappingActors,
		// TArray<AActor*>{AvatarActor,MouseHitActor}, MouseHitActor->GetActorLocation(),Radius);
		UMageAbilitySystemLibrary::GetLivingActorInCollisionShape(GetAvatarActorFromActorInfo(),OverlappingActors, TArray<AActor*>{AvatarActor,TargetingActor}, TargetingActor->GetActorLocation(),EColliderShape::Sphere,false,Radius);
		
		// 获取距离最近的Actor
		//TargetNum = FMath::Min(GetAbilityLevel(), MaxTargetNum);
		UMageAbilitySystemLibrary::GetClosestActors(OverlappingActors,OutAdditionalTargets,TargetingActor->GetActorLocation(), TargetNum);
	}
	
	// AdditionalTargets 绑定OnDeath委托，当 AdditionalTargets 死亡时，调用回调,强制关闭GC
	for(AActor* Target : OutAdditionalTargets)
	{
		if(ICombatInterface* CombatInterface = Cast<ICombatInterface>(Target))
		{
			if(!CombatInterface->GetOnDeathDelegate().IsAlreadyBound(this,&UMageGA_Beam::OnTargetDiedCallback))
			{
				CombatInterface->GetOnDeathDelegate().AddDynamic(this,&UMageGA_Beam::OnTargetDiedCallback);
			}
		}
	}
}
