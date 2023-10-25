#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
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
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                          const FHitResult& SweepResult);
	
public:
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true)) //暴露给该类的SpawnActor
	FGameplayEffectSpecHandle DamageEffectSpecHandle;


private:
	UPROPERTY(EditAnywhere, Category = "Mage_Projectile")
	float LifeSpan = 5.f;

	bool bHit = false; // 用于标记客户端上的 Projectile 是否已经重叠过了
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;
	

	/** GameplayCue */
	UPROPERTY(EditAnywhere, Category = "Mage_GameplayCue")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, Category = "Mage_GameplayCue")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, Category = "Mage_GameplayCue")
	TObjectPtr<USoundBase> FlySound;
	UPROPERTY()
	TObjectPtr<UAudioComponent> FlyAudioComponent;
	
};
