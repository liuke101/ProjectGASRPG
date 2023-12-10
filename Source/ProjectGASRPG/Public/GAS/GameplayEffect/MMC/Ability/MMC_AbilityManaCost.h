// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_AbilityManaCost.generated.h"

/** 计算技能消耗 */
UCLASS()
class PROJECTGASRPG_API UMMC_AbilityManaCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
public:
	UMMC_AbilityManaCost();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
