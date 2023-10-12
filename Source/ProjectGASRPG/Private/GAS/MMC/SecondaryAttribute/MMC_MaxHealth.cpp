#include "GAS/MMC/SecondaryAttribute/MMC_MaxHealth.h"
#include "GAS/MageAttributeSet.h"
#include "Interface/CombatInterface.h"
UMMC_MaxHealth::UMMC_MaxHealth()
{
	StrengthDef.AttributeToCapture = UMageAttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;

	StaminaDef.AttributeToCapture = UMageAttributeSet::GetStaminaAttribute();
	StaminaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StaminaDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(StrengthDef);
	RelevantAttributesToCapture.Add(StaminaDef);
}

float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 获取Tag
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Strength = 0.0f;
	float Stamina = 0.0f;
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
	GetCapturedAttributeMagnitude(StaminaDef, Spec, EvaluationParameters, Stamina);

	Strength = FMath::Max<float>(Strength,0.0f);
	Stamina = FMath::Max<float>(Stamina,0.0f);
	
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetPlayerLevel();

	return (Strength * 2.0f + Stamina * 19.4f) + PlayerLevel * 10.0f;
}