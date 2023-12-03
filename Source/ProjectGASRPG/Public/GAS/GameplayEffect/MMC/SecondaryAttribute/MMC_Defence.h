#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_Defence.generated.h"

UCLASS()
class PROJECTGASRPG_API UMMC_Defence : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_Defence();

	/** 重载  */
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	/** 声明捕获属性 */
	DECLARE_ATTRIBUTE_CAPTUREDEF(Stamina);
};
