#include "GAS/GameplayEffect/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"

struct MageDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Intelligence);
	
	MageDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, Intelligence, Target, false);
	}
};

static const MageDamageStatics& DamageStatics()
{
	static MageDamageStatics DmgStatics;
	return DmgStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().IntelligenceDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& AbilitySpec = ExecutionParams.GetOwningSpec();

	// 获取Tag
	const FGameplayTagContainer* SourceTags = AbilitySpec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = AbilitySpec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;
	
	
	float Intelligence = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IntelligenceDef, EvaluationParameters, Intelligence);
	Intelligence = FMath::Max<float>(0.0f,Intelligence);

	const float Damage = 5.0f + Intelligence * 0.25f;
	
	const FGameplayModifierEvaluatedData EvaluatedData(DamageStatics().IntelligenceProperty, EGameplayModOp::Additive,Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
