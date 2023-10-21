#include "GAS/MMC/SecondaryAttribute//MMC_MaxPhysicalAttack.h"
#include "GAS/MageAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_MaxPhysicalAttack::UMMC_MaxPhysicalAttack()
{
	StrengthDef.AttributeToCapture = UMageAttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;

	VigorDef.AttributeToCapture = UMageAttributeSet::GetVigorAttribute();
	VigorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VigorDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(StrengthDef);
	RelevantAttributesToCapture.Add(VigorDef);
}

float UMMC_MaxPhysicalAttack::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 获取Tag
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Strength = 0.0f;
	float Vigor = 0.0f;
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);

	Strength = FMath::Max<float>(Strength,0.0f);
	Vigor = FMath::Max<float>(Vigor,0.0f);
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetCharacterLevel();

	return (Strength * 1.3f + Vigor * 2.5f) + PlayerLevel;
}
