// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS/MageAbilityTypes.h"
#include "MageIceBlast.generated.h"

class UNiagaraSystem;
class UCapsuleComponent;

UCLASS()
class PROJECTGASRPG_API AMageIceBlast : public AActor
{
	GENERATED_BODY()

public:
	AMageIceBlast();

protected:
	
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true)) //暴露给该类的SpawnActor
	FDamageEffectParams DamageEffectParams;


protected:
	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);
private:
	UPROPERTY(EditAnywhere, Category = "Mage")
	float LifeSpan = 5.f;

	UPROPERTY(EditAnywhere,Category = "Mage")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(EditAnywhere, Category = "Mage")
	TSubclassOf<UNiagaraSystem> AbilityNiagara;
};




