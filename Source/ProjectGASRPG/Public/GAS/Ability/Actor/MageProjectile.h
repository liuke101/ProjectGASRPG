#pragma once

#include "CoreMinimal.h"
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

	
private:
	UPROPERTY(EditAnywhere)
	float LifeSpan = 5.f;

	bool bHit = false;
	
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
