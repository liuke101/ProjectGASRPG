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

	UFUNCTION(BlueprintCallable, Category = "Mage_GA|ProjectileAbility")
	void SpawnProjectile(const FVector& TargetLocation, const FGameplayTag& AttackSocketTag, const bool bOverridePitch = false, const float PitchOverride = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Mage_GA|ProjectileAbility")
	void SpawnMultiProjectiles(AActor* HomingTarget, const FVector& TargetLocation,const int32 ProjectilesNum,const FGameplayTag& AttackSocketTag,const bool bOverridePitch  = false, const float PitchOverride = 0.0f);

public:
	UFUNCTION(BlueprintCallable, Category = "Mage_GA|ProjectileAbility")
	int32 GetSpawnProjectilesNum(int32 AbilityLevel) const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA")
	TSubclassOf<AMageProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Mage_GA")
	int32 MaxProjectilesNum = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_GA|ProjectileAbility")
	float SpawnSpread = 90.0f;
};
