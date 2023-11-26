// 

#pragma once

#include "CoreMinimal.h"
#include "MageDamageGameplayAbility.h"
#include "BeamGA.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UBeamGA : public UMageDamageGameplayAbility
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void StoreMouseDataInfo(const FHitResult& HitResult);

protected:
	UPROPERTY(BlueprintReadWrite,Category = "Mage_GA|Beam")
	FVector MouseHitLocation;

	UPROPERTY(BlueprintReadWrite,Category = "Mage_GA|Beam")
	TObjectPtr<AActor> MouseHitActor;

	
};
