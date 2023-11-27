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
	UFUNCTION(BlueprintCallable,Category = "Mage_GA|Beam")
	void StoreMouseDataInfo(const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable,Category = "Mage_GA|Beam")
	void TraceFirstTarget(const FVector& TargetLocation);
protected:
	UPROPERTY(BlueprintReadWrite,Category = "Mage_GA|Beam")
	FVector MouseHitLocation;

	UPROPERTY(BlueprintReadWrite,Category = "Mage_GA|Beam")
	TObjectPtr<AActor> MouseHitActor;

	
};
