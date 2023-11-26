#include "GAS/Ability/MageProjectileSpellGA.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/Actor//MageProjectile.h"
#include "Interface/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

int32 UMageProjectileSpellGA::GetSpawnProjectilesNum(int32 AbilityLevel) const
{
	// 限制最大数量
	return FMath::Min(AbilityLevel,MaxProjectilesNum);
}

void UMageProjectileSpellGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMageProjectileSpellGA::SpawnProjectile(const FVector& TargetLocation,const FGameplayTag& AttackSocketTag, const bool bOverridePitch, const float PitchOverride)
{
	/* 只在服务器生成火球，客户端的效果通过服务器复制 */
	if(const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority(); !bIsServer) return;
	
	/** Spawn Projectile */
	if(const ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
	{
		checkf(ProjectileClass, TEXT("%s的ProjectileClass为空，请在蓝图中设置"), *GetName());
		checkf(DamageEffectClass, TEXT("%s的DamageEffectClass为空，请在蓝图中设置"), *GetName());

		/** 获取AvatarActor的WeaponSocket位置, 作为火球Spawn位置 */
		const FVector WeaponSocketLocation = CombatInterface->Execute_GetWeaponSocketLocationByTag(GetAvatarActorFromActorInfo(), AttackSocketTag);
		FRotator WeaponSocketRotation = (TargetLocation - WeaponSocketLocation).ToOrientationRotator(); //旋转到向量指向方向
		//WeaponSocketRotation.Pitch = 0.f;  //如果想让火球水平发射，可以取消注释

		if(bOverridePitch)
		{
			WeaponSocketRotation.Pitch = PitchOverride;
		}
		
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(WeaponSocketLocation);
		SpawnTransform.SetRotation(WeaponSocketRotation.Quaternion());
		
		/** SpawnActorDeferred 函数可以延迟 Spawn, 直到调用 FinishSpawning， 在这期间我们可以对Projectile对象进行设置*/
		AMageProjectile* MageProjectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		/** 设置 Projectile 的 DamageEffectParams(此时还不确定TargetActor，需要在发射物触发Overlap时再设置) */
		MageProjectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefault();
		
		MageProjectile->FinishSpawning(SpawnTransform);
	}
}

void UMageProjectileSpellGA::SpawnMultiProjectiles(AActor* HomingTarget, const FVector& TargetLocation, int32 ProjectilesNum,const FGameplayTag& AttackSocketTag, const bool bOverridePitch, const float PitchOverride)
{
	/* 只在服务器生成火球，客户端的效果通过服务器复制 */
	if(const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority(); !bIsServer) return;
	
	/** Spawn Projectile */
	if(const ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
	{
		checkf(ProjectileClass, TEXT("%s的ProjectileClass为空，请在蓝图中设置"), *GetName());
		checkf(DamageEffectClass, TEXT("%s的DamageEffectClass为空，请在蓝图中设置"), *GetName());

		/** 获取AvatarActor的WeaponSocket位置, 作为火球Spawn位置 */
		const FVector WeaponSocketLocation = CombatInterface->Execute_GetWeaponSocketLocationByTag(GetAvatarActorFromActorInfo(), AttackSocketTag);
		FRotator WeaponSocketRotation = (TargetLocation - WeaponSocketLocation).ToOrientationRotator(); //旋转到向量指向方向
		//WeaponSocketRotation.Pitch = 0.f;  //如果想让火球水平发射，可以取消注释

		/** 重载发射角度，效果是发射位置上下偏移 */
		if(bOverridePitch)
		{
			WeaponSocketRotation.Pitch = PitchOverride;
		}
		
		const FVector Forward = WeaponSocketRotation.Vector();
		
		/** 获取均匀分布的Rotator */
		TArray<FRotator> Rotators = UMageAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, SpawnSpread, ProjectilesNum);
		
		for(auto Rotator : Rotators)
		{
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(WeaponSocketLocation);
			SpawnTransform.SetRotation(Rotator.Quaternion());
			
			/** SpawnActorDeferred 函数可以延迟 Spawn, 直到调用 FinishSpawning， 在这期间我们可以对Projectile对象进行设置*/
			AMageProjectile* Projectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			
			/** 设置 Projectile 的 DamageEffectParams(此时还不确定TargetActor，需要在发射物触发Overlap时再设置) */
			Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefault();

			/** 瞄准目标 */
			// 开启追踪
			Projectile->ProjectileMovement->bIsHomingProjectile = bIsHomingProjectile; 
			// 设置加速度
			Projectile->ProjectileMovement->HomingAccelerationMagnitude = FMath::RandRange(MinHomingAcceleration, MaxHomingAcceleration);
			// 设置目标
			if(HomingTarget)
			{
				if(HomingTarget->Implements<UCombatInterface>())
				{
					Projectile->ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
				}
				else
				{
					// 当目标为普通Actor时，在目标位置创建一个SceneComponent作为目标
					// HomingTargetComponent是一个弱指针，我们在AMageProjectile类中创建了一个HomingTargetSceneComponent,并标记为UPROPERTY(),保证可以正常GC。
					Projectile->HomingTargetSceneComponent = NewObject<USceneComponent>(USceneComponent::StaticClass());
					Projectile->HomingTargetSceneComponent->SetWorldLocation(HomingTarget->GetActorLocation());
					Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTargetSceneComponent;
				}
			}
			
			Projectile->FinishSpawning(SpawnTransform);
		}
		
		
	}
}


