#pragma once

#include "CoreMinimal.h"
#include "MageGameplayAbility.h"
#include "MageProjectileSpellGA.generated.h"

class AMageProjectile;

UCLASS()
class PROJECTGASRPG_API UMageProjectileSpellGA : public UMageGameplayAbility
{
	GENERATED_BODY()

protected:
	// 对应蓝图中的 Event ActivateAbility
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA")
	TSubclassOf<AMageProjectile> ProjectileClass;

	
	
	
};
