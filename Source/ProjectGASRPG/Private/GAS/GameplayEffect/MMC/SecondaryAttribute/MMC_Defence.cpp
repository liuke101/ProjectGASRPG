#include "GAS/GameplayEffect/MMC/SecondaryAttribute/MMC_Defence.h"
#include "GAS/MageAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_Defence::UMMC_Defence()
{
	//定义捕获属性，第四个参数是Snapshot
	DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, Stamina, Target, false);
	
	// 捕捉与计算相关的属性
	// 可以通过 GetAttributeCaptureDefinitions() 访问
	RelevantAttributesToCapture.Add(StaminaDef);
}


float UMMC_Defence::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 获取Tag
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// 在聚合器评估中使用的数据
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 从捕获属性中获取BaseValue
	float Stamina = 0.0f;
	GetCapturedAttributeMagnitude(StaminaDef, Spec, EvaluationParameters, Stamina);
	Stamina = FMath::Max<float>(Stamina,0.0f);

	// 自定义的接口，获取角色等级
	const ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetCharacterLevel();

	// 应用计算公式
	return (Stamina * 4.8f) + PlayerLevel;
}