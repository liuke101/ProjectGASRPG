#include "GAS/Ability/MageProjectileSpellGA.h"

#include "GAS/Ability/Actor//MageProjectile.h"
#include "Interface/CombatInterface.h"

void UMageProjectileSpellGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	/* 只在服务器生成火球，客户端的效果通过服务器复制 */
	if(const bool bIsServer = HasAuthority(&ActivationInfo); !bIsServer) return; 

	/* Spawn火球 */
	if(ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
	{
		checkf(ProjectileClass, TEXT("%s的ProjectileClass为空，请在蓝图中设置"), *GetName());
		
		/* 获取AvatarActor的武器顶端Socket位置, 作为火球Spawn位置 */
		const FVector WeaponSocketLocation = CombatInterface->GetWeaponSocketLocation();
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(WeaponSocketLocation);
		//TODO: 设置旋转
		
		//我们想在要击中的Actor身上设置GameplayEffect，如果想要在actor身上设置变量或其他可以使用该函数。
		//意思就是延迟Spawn，直到调用FinishSpawning
		AMageProjectile* MageProjectile = GetWorld()->SpawnActorDeferred<AMageProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(),Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		//TODO:设置GameplayEffect来造成伤害
		
		MageProjectile->FinishSpawning(SpawnTransform);
	}
	
	
	
	
}
