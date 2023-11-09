#pragma once

#include "CoreMinimal.h"
#include "MageDamageGameplayAbility.h"
#include "MageProjectileSpellGA.generated.h"

class AMageProjectile;

UCLASS()
class PROJECTGASRPG_API UMageProjectileSpellGA : public UMageDamageGameplayAbility
{
	GENERATED_BODY()

protected:
	// 对应蓝图中的 Event ActivateAbility, 激活时调用
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "Mage_GA|ProjectileSpellAbility")
	void SpawnProjectile(const FVector& TargetLocation, const FGameplayTag& SocketTag);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA")
	TSubclassOf<AMageProjectile> ProjectileClass;
	
};
