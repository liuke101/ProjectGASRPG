﻿#include "GAS/Ability/MageProjectileSpellGA.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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
		
		/**
		 * 我们想在要击中的Actor身上设置GameplayEffect，如果想要在actor身上设置变量或其他可以使用 SpawnActorDeferred 函数。、
		 * 该函数可以延迟Spawn，直到调用FinishSpawning
		 */
		AMageProjectile* MageProjectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		/**
		 * 造成伤害
		 * - 设置 Projectile 的 DamageEffectParams(此时还不确定TargetActor，需要在发射物触发Overlap时再设置)
		 */
		MageProjectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefault();
		
		MageProjectile->FinishSpawning(SpawnTransform);
	}
}

void UMageProjectileSpellGA::SpawnMultiProjectiles(AActor* HomingTarget, const FVector& TargetLocation, const int32 ProjectilesNum,const FGameplayTag& AttackSocketTag, const bool bOverridePitch, const float PitchOverride)
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
		
		const FVector Forward = WeaponSocketRotation.Vector();
		
		/** 获取均匀分布的Rotator */
		TArray<FRotator> Rotators = UMageAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, SpawnSpread, ProjectilesNum);
		
		for(auto Rotator : Rotators)
		{
			// 生成位置
			FVector SpawnLocation = WeaponSocketLocation + FVector(0.f,0.f,45.f);
			
			FTransform SpawnTransform;
			SpawnTransform.SetLocation(SpawnLocation);
			SpawnTransform.SetRotation(Rotator.Quaternion());
			
			/**
			 * 我们想在要击中的Actor身上设置GE，如果想要在Actor身上设置变量或其他可以使用 SpawnActorDeferred 函数。、
			 * 该函数可以延迟Spawn，直到调用FinishSpawning
			 */
			AMageProjectile* MageProjectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			
			/**
			 * 造成伤害
			 * - 设置 Projectile 的 DamageEffectParams(此时还不确定TargetActor，需要在发射物触发Overlap时再设置)
			 */
			MageProjectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefault();
			
			MageProjectile->FinishSpawning(SpawnTransform);
		}
		
		
	}
}


