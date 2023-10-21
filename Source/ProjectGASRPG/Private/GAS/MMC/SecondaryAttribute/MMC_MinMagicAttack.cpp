// 


#include "GAS/MMC/SecondaryAttribute/MMC_MinMagicAttack.h"
#include "GAS/MageAttributeSet.h"
#include "Interface/CombatInterface.h"
UMMC_MinMagicAttack::UMMC_MinMagicAttack()
{
	IntelligenceDef.AttributeToCapture = UMageAttributeSet::GetIntelligenceAttribute();
	IntelligenceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	IntelligenceDef.bSnapshot = false;

	VigorDef.AttributeToCapture = UMageAttributeSet::GetVigorAttribute();
	VigorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VigorDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(IntelligenceDef);
	RelevantAttributesToCapture.Add(VigorDef);
}

float UMMC_MinMagicAttack::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 获取Tag
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Intelligence = 0.0f;
	float Vigor = 0.0f;
	GetCapturedAttributeMagnitude(IntelligenceDef, Spec, EvaluationParameters, Intelligence);
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);

	Intelligence = FMath::Max<float>(Intelligence,0.0f);
	Vigor = FMath::Max<float>(Vigor,0.0f);
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetCharacterLevel();

	return (Intelligence * 2.2f + Vigor * 1.7f) + PlayerLevel;
}