#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MageEffectActor.generated.h"

class USphereComponent;

UCLASS()
class PROJECTGASRPG_API AMageEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AMageEffectActor();

	UFUNCTION()
	virtual void OnSphereComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	
	UFUNCTION()
	virtual void OnSphereComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
	
	UPROPERTY()
	TObjectPtr<USphereComponent> SphereComponent;
};
