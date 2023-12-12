#pragma once

#include "CoreMinimal.h"
#include "MageGA_Damage.h"
#include "MageGA_Projectile.generated.h"

class AMageProjectile;

/** 投掷物技能 */
UCLASS()
class PROJECTGASRPG_API UMageGA_Projectile : public UMageGA_Damage
{
	GENERATED_BODY()
protected:
	// 对应蓝图中的 Event ActivateAbility, 激活时调用
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category = "Mage_GA|Projectile")
	void SpawnProjectile(const FVector& TargetLocation, const FGameplayTag& AttackSocketTag, const bool bOverridePitch = false, const float PitchOverride = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Mage_GA|Projectile")
	void SpawnMultiProjectiles(AActor* HomingTarget, const FVector& TargetLocation,int32 ProjectilesNum,const FGameplayTag& AttackSocketTag,const bool bOverridePitch  = false, const float PitchOverride = 0.0f);

public:
	UFUNCTION(BlueprintCallable, Category = "Mage_GA|Projectile")
	int32 GetSpawnProjectilesNum(int32 AbilityLevel) const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GA|Projectile")
	TSubclassOf<AMageProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Mage_GA|Projectile")
	int32 MaxProjectilesNum = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_GA|Projectile")
	float SpawnSpread = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_GA|Projectile")
	float MinHomingAcceleration = 1600.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Mage_GA|Projectile")
	float MaxHomingAcceleration = 3200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_GA|Projectile")
	bool bIsHomingProjectile = true;
};
