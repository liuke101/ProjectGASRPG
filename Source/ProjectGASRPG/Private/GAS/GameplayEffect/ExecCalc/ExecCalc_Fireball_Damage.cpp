#include "GAS/GameplayEffect/ExecCalc/ExecCalc_Fireball_Damage.h"
#include "AbilitySystemComponent.h"
#include "MathUtil.h"
#include "GAS/MageAbilitySystemGlobals.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageAbilityTypes.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"
#include "GAS/Data/CharacterClassDataAsset.h"

/** 该结构体用于捕获属性的声明和定义 */
struct MageDamageStatics
{
	/** 声明捕获属性 */
	/** Source */
	DECLARE_ATTRIBUTE_CAPTUREDEF(MinMagicAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxMagicAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);

	/** Target */
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	
	MageDamageStatics()
	{
		/** 定义捕获属性 */
		/** Source */
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, MinMagicAttack, Source, true); 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, MaxMagicAttack, Source, true); 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, CriticalHitChance, Source, true); 

		/** Target */
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, Defense, Target, false); 
	}
};

static const MageDamageStatics& DamageStatics()
{
	static MageDamageStatics DmgStatics;
	return DmgStatics;
}

UExecCalc_Fireball_Damage::UExecCalc_Fireball_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().MinMagicAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().MaxMagicAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitChanceDef);
	
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
}

void UExecCalc_Fireball_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	AActor* SourceAvatarActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatarActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	/** 获取GameplayEffectSpec */
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

	/** 获取Tag */
	const FGameplayTagContainer* SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	/** Set By Caller 获取技能基本伤害，在GA中设置 */
	const float BaseDamage = EffectSpec.GetSetByCallerMagnitude(FMageGameplayTags::Get().SetByCaller_Damage, true);

	/** 捕获属性 */
	float MinMagicAttack = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MinMagicAttackDef, EvaluationParameters, MinMagicAttack);
	MinMagicAttack = FMath::Max<float>(0.0f,MinMagicAttack);
	
	float MaxMagicAttack = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxMagicAttackDef, EvaluationParameters, MaxMagicAttack);
	MaxMagicAttack = FMath::Max<float>(0.0f,MaxMagicAttack);
	
	float CriticalHitChance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef, EvaluationParameters, CriticalHitChance);
	CriticalHitChance = FMath::Max<float>(0.0f,CriticalHitChance);
	
	float Defense = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluationParameters, Defense);
	Defense = FMath::Max<float>(0.0f,Defense);

	/** 获得技能等级：根据Ability.Mage.Fireball标签找到火球的Ability */
	const int32 AbilityLevel = UMageAbilitySystemLibrary::GetAbilityLevelFromTag(SourceASC, FMageGameplayTags::Get().Ability_Mage_Fireball);
	
	/** 是否暴击 */
	const bool bIsCriticalHit = FMath::RandRange(0.f, 1.f) <= CriticalHitChance;

	/** 获取EffectContext，同步暴击状态 */
	FGameplayEffectContextHandle EffectContextHandle = EffectSpec.GetContext();
	UMageAbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle, bIsCriticalHit);
	
	/** 根据技能等级读取曲线表格，获取攻击加成和暴击伤害数据 */
	const UCharacterClassDataAsset* CharacterClassDataAsset = UMageAbilitySystemLibrary::GetCharacterClassDataAsset(SourceAvatarActor);
	const float AttackBonus = CharacterClassDataAsset->CalcDamageCurveTable->FindCurve(FName(TEXT("ExecCalc.AttackBonus")),FString(),true)->Eval(AbilityLevel);
	const float CriticalHitDamage = CharacterClassDataAsset->CalcDamageCurveTable->FindCurve(FName(TEXT("ExecCalc.CriticalHitDamage")),FString(),true)->Eval(AbilityLevel);
	
	/** 伤害公式 */
	float Damage = BaseDamage + FMath::RandRange(MinMagicAttack, MaxMagicAttack) * AttackBonus - Defense;
	Damage = FMath::Max<float>(0.0f,Damage); //伤害最小为0, 否则会出现负数
	Damage = bIsCriticalHit ? Damage * CriticalHitDamage : Damage;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("火球术 | 伤害: %f | 技能等级: %d"), Damage, AbilityLevel));

	/** 修改MetaDamage属性 */
	const FGameplayModifierEvaluatedData EvaluatedData(UMageAttributeSet::GetMetaDamageAttribute(), EGameplayModOp::Additive, Damage);

	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
