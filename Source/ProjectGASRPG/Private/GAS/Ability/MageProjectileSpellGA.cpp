#include "GAS/Ability/MageProjectileSpellGA.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/Actor//MageProjectile.h"
#include "Interface/CombatInterface.h"

void UMageProjectileSpellGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMageProjectileSpellGA::SpawnProjectile(const FVector& TargetLocation)
{
	/* 只在服务器生成火球，客户端的效果通过服务器复制 */
	if(const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority(); !bIsServer) return;
	
	/* Spawn Projectile */
	if(ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
	{
		checkf(ProjectileClass, TEXT("%s的ProjectileClass为空，请在蓝图中设置"), *GetName());
		checkf(DamageEffectClass, TEXT("%s的DamageEffectClass为空，请在蓝图中设置"), *GetName());

		/* 获取AvatarActor的武器顶端Socket位置, 作为火球Spawn位置 */
		const FVector WeaponSocketLocation = CombatInterface->GetWeaponSocketLocation();
		FRotator WeaponSocketRotation = (TargetLocation - WeaponSocketLocation).Rotation();
		//WeaponSocketRotation.Pitch = 0.f;  //如果想让火球水平发射，可以取消注释
		
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(WeaponSocketLocation);
		SpawnTransform.SetRotation(WeaponSocketRotation.Quaternion());
		
		//我们想在要击中的Actor身上设置GameplayEffect，如果想要在actor身上设置变量或其他可以使用 SpawnActorDeferred 函数。
		//意思就是延迟Spawn，直到调用FinishSpawning
		AMageProjectile* MageProjectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		/** 造成伤害 */
		//作者写的比较麻烦，我这里直接使用MakeOutgoingGameplayEffectSpec
		//const UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningActorFromActorInfo());
		//或const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		//const FGameplayEffectSpecHandle DamageEffectSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), SourceASC->MakeEffectContext());

		FGameplayEffectSpecHandle DamageEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());

		//SetByCaller Modifier计算技能伤害
		const FMageGameplayTags& GameplayTagsInstance = FMageGameplayTags::Get();
		const float ScaledDamage = Damage.GetValueAtLevel(GetAbilityLevel()); //基于技能等级获取曲线表格的值
		DamageEffectSpecHandle = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageEffectSpecHandle, GameplayTagsInstance.MetaAttribute_Damage, ScaledDamage); //设置
		
		MageProjectile->DamageEffectSpecHandle = DamageEffectSpecHandle;
		
		MageProjectile->FinishSpawning(SpawnTransform);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("1键 释放火球, 伤害: %f"), ScaledDamage));
	}
}
