#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_Damage.generated.h"


/** Execution Calculation 计算伤害 */
UCLASS()
class PROJECTGASRPG_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	static void MakeAttributeTag_To_CaptureDef(TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition>& AttributeTag_To_CaptureDef);

	/**
	 * 计算Debuff
	 * Set By Caller 获取Debuff信息，在GA中设置, 由 ApplyDamageEffect() 函数分配SetByCaller
	 */
	void CalcDebuff(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvaluationParameters, const FGameplayEffectSpec& EffectSpec,FGameplayEffectContextHandle& EffectContextHandle, const TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition>& AttributeTag_To_CaptureDef) const;
};
