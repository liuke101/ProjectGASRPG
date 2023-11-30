#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "GAS/MageAbilityTypes.h"
#include "MageProjectile.generated.h"

class UNiagaraSystem;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class PROJECTGASRPG_API AMageProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMageProjectile();

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true)) //暴露给该类的SpawnActor
	FDamageEffectParams DamageEffectParams;

	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

private:
	UPROPERTY(EditAnywhere, Category = "Mage_Projectile")
	float LifeSpan = 5.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;
	

	/** GameplayCue */
	UPROPERTY(EditAnywhere, Category = "Mage_GameplayCue")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, Category = "Mage_GameplayCue")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Mage_GameplayCue")
	TObjectPtr<USoundBase> FlySound;

	UFUNCTION()
	void DestroyFlyAudioComponent() const;
	UPROPERTY()
	TObjectPtr<UAudioComponent> FlyAudioComponent;
};
