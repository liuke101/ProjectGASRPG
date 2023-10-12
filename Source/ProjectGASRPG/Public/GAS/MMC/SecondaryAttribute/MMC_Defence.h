// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_Defence.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMMC_Defence : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_Defence();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition StaminaDef;
};
