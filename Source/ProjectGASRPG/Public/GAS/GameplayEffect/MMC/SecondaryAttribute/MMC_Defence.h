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
	/** 声明和定义捕获属性也可以使用DECLARE_ATTRIBUTE_CAPTUREDEF和DEFINE_ATTRIBUTE_CAPTUREDEF宏，这里手动声明和定义了捕获属性 */
	FGameplayEffectAttributeCaptureDefinition StaminaDef;
};
