#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_AbilityVitalityCost.generated.h"


UCLASS()
class PROJECTGASRPG_API UMMC_AbilityVitalityCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	UMMC_AbilityVitalityCost();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
