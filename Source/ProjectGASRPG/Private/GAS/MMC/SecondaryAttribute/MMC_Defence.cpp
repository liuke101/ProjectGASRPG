#include "GAS/MMC/SecondaryAttribute/MMC_Defence.h"
#include "GAS/MageAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_Defence::UMMC_Defence()
{
	StaminaDef.AttributeToCapture = UMageAttributeSet::GetStaminaAttribute();
	StaminaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StaminaDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(StaminaDef);
}


float UMMC_Defence::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 获取Tag
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Stamina = 0.0f;
	GetCapturedAttributeMagnitude(StaminaDef, Spec, EvaluationParameters, Stamina);

	Stamina = FMath::Max<float>(Stamina,0.0f);
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetPlayerLevel();

	return (Stamina * 4.8f) + PlayerLevel;
}