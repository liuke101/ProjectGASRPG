#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MageEffectActor.generated.h"

class UGameplayEffect;
class USphereComponent;

UCLASS()
class PROJECTGASRPG_API AMageEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AMageEffectActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Mage_Effects")
	void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass);
	
	UPROPERTY(EditAnywhere,  Category = "Mage_Effects")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;
};
