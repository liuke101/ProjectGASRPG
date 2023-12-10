// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_AbilityHealthCost.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMMC_AbilityHealthCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	UMMC_AbilityHealthCost();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
};
