#include "GAS/Ability/MageProjectileSpellGA.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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
	/** 如果投掷物数量为1，直接生成单个投掷物 */
	if(ProjectilesNum <= 1)
	{
		SpawnProjectile(TargetLocation, AttackSocketTag, bOverridePitch, PitchOverride);
		return;
	}
	
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
		
		const FVector ForwardVector = WeaponSocketRotation.Vector();
		// 绕轴旋转(Z轴（UpVector）旋转方向遵循左手定则，顺时针为正)
		const FVector LeftOfSpread = ForwardVector.RotateAngleAxis(-SpawnSpread/2.f, FVector::UpVector);
		const float DeltaSpread = SpawnSpread / (ProjectilesNum-1); //每个投掷物之间的角度差(例如有三个投掷物时，间隔为45度)
		
		for(int32 i=0;i<ProjectilesNum;i++)
		{
			const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i, FVector::UpVector);
			const FVector SpawnLocation = WeaponSocketLocation + FVector(0,0,10); //发射位置抬高一点
			UKismetSystemLibrary::DrawDebugArrow(GetOwningActorFromActorInfo(),SpawnLocation, SpawnLocation  + Direction *100.0f, 10.0f, FLinearColor::Red, 10.0f, 5.0f);
		}
		
			
		// FTransform SpawnTransform;
		// SpawnTransform.SetLocation(WeaponSocketLocation);
		// SpawnTransform.SetRotation(WeaponSocketRotation.Quaternion());
		//
		// /**
		//  * 我们想在要击中的Actor身上设置GameplayEffect，如果想要在actor身上设置变量或其他可以使用 SpawnActorDeferred 函数。、
		//  * 该函数可以延迟Spawn，直到调用FinishSpawning
		//  */
		// AMageProjectile* MageProjectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		//
		// /**
		//  * 造成伤害
		//  * - 设置 Projectile 的 DamageEffectParams(此时还不确定TargetActor，需要在发射物触发Overlap时再设置)
		//  */
		// MageProjectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefault();
		//
		// MageProjectile->FinishSpawning(SpawnTransform);
	}
}


