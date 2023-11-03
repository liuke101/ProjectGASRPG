#include "GAS/GameplayEffect/ExecCalc/ExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageAbilityTypes.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/CombatInterface.h"

/** 该结构体用于捕获属性的声明和定义 */
struct MageDamageStatics
{
	/** 声明捕获属性 */
	/** Source */
	DECLARE_ATTRIBUTE_CAPTUREDEF(MinPhysicalAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxPhysicalAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MinMagicAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxMagicAttack);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalHitChance);

	/** Target */
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IceResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalResistance);

	/** 属性 Tag 与 CaptureDef 的映射 */
	TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> AttributeTag_To_CaptureDef;
	
	MageDamageStatics()
	{
		/** 定义捕获属性 */
		/** Source */
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, MinPhysicalAttack, Source, true); 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, MaxPhysicalAttack, Source, true); 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, MinMagicAttack, Source, true); 
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, MaxMagicAttack, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, CriticalHitChance, Source, true); 

		/** Target */
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, Defense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, FireResistance, Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, IceResistance, Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, LightningResistance, Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UMageAttributeSet, PhysicalResistance, Target, false)

		const FMageGameplayTags& Tags = FMageGameplayTags::Get();
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_MinMagicAttack, MinMagicAttackDef);
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_MaxMagicAttack, MaxMagicAttackDef);
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_CriticalHitChance, CriticalHitChanceDef);
		
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_Defense, DefenseDef);
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Fire, FireResistanceDef);
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Ice, IceResistanceDef);
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Lightning, LightningResistanceDef);
		AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Physical, PhysicalResistanceDef);
	}
};

static const MageDamageStatics& DamageStatics()
{
	static MageDamageStatics DmgStatics;
	return DmgStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().MinPhysicalAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().MaxPhysicalAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().MinMagicAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().MaxMagicAttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalHitChanceDef);
	
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().IceResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().LightningResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().PhysicalResistanceDef);
	
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
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

	/** 捕获属性, 法师捕获法术攻击力，其他捕获物理攻击力 */
	float MinAttack= 0.0f;
	float MaxAttack = 0.0f;

	if(Cast<ICombatInterface>(SourceAvatarActor)->GetCharacterClass() == ECharacterClass::Mage)
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MinMagicAttackDef, EvaluationParameters, MinAttack);
		MinAttack = FMath::Max<float>(0.0f,MinAttack);
	
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxMagicAttackDef, EvaluationParameters, MaxAttack);
		MaxAttack = FMath::Max<float>(0.0f,MaxAttack);
	}
	else
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MinPhysicalAttackDef, EvaluationParameters, MinAttack);
		MinAttack = FMath::Max<float>(0.0f,MinAttack);
	
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxPhysicalAttackDef, EvaluationParameters, MaxAttack);
		MaxAttack = FMath::Max<float>(0.0f,MaxAttack);
	}
	
	float CriticalHitChance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef, EvaluationParameters, CriticalHitChance);
	CriticalHitChance = FMath::Max<float>(0.0f,CriticalHitChance);
	
	float Defense = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluationParameters, Defense);
	Defense = FMath::Max<float>(0.0f,Defense);

	/** Set By Caller 获取技能类型伤害，在GA中设置 */
	float TypeDamage = 0;
	// 遍历所有伤害类型
	for(auto& Pair : FMageGameplayTags::Get().DamageTypeTag_To_ResistanceTag)
	{
		/**
		 * 获取伤害类型对应的magnitude(在GA中通过AssignTagSetByCallerMagnitude设置)
		 * 如果伤害为0, 跳过该类型的计算(当GA中的DamageTypeTag_To_AbilityDamage映射没有设置对应的伤害类型时，自然无法通过SetByCaller获取magnitude,GetSetByCallerMagnitude会返回0)
		 */
		float TypeDamageMagnitude = EffectSpec.GetSetByCallerMagnitude(Pair.Key,false, 0); //找不到就返回0, 指明不报错
		if(TypeDamageMagnitude == 0) continue; 
		
		const FGameplayTag ResistanceTag = Pair.Value; //获取伤害类型对应的抗性Tag

		checkf(MageDamageStatics().AttributeTag_To_CaptureDef.Contains(ResistanceTag), TEXT("抗性Tag %s 没有对应的捕获属性定义"), *ResistanceTag.ToString());
	    const FGameplayEffectAttributeCaptureDefinition CaptureDef = MageDamageStatics().AttributeTag_To_CaptureDef[ResistanceTag]; //获取抗性Tag对应的捕获属性定义

		// 获取抗性属性
		float TypeResistance = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, TypeResistance);
		TypeResistance = FMath::Clamp<float>(TypeResistance, 0.0f,1.0f);

		// 计算最终的类型伤害
		// TODO:这里用+=会叠加所有类型的伤害，如果只想计算一种类型的伤害，可以用=，具体是否会有bug，等遇到多属性伤害的时候再测试
		TypeDamage += TypeDamageMagnitude * (1 - TypeResistance);
	}

	/** 获得技能等级 */
	const int32 AbilityLevel = EffectSpec.GetLevel();
	
	/** 根据技能等级读取曲线表格，获取攻击加成和暴击伤害数据 */
	const UCharacterClassDataAsset* CharacterClassDataAsset = UMageAbilitySystemLibrary::GetCharacterClassDataAsset(SourceAvatarActor);
	const float AttackBonus = CharacterClassDataAsset->CalcDamageCurveTable->FindCurve(FName(TEXT("ExecCalc.AttackBonus")),FString(),true)->Eval(AbilityLevel);
	const float CriticalHitDamage = CharacterClassDataAsset->CalcDamageCurveTable->FindCurve(FName(TEXT("ExecCalc.CriticalHitDamage")),FString(),true)->Eval(AbilityLevel);
	
	/** 是否暴击 */
	const bool bIsCriticalHit = FMath::RandRange(0.f, 1.f) <= CriticalHitChance;

	/** 获取EffectContext，同步暴击状态 */
	FGameplayEffectContextHandle EffectContextHandle = EffectSpec.GetContext();
	UMageAbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle, bIsCriticalHit);

	/** 伤害公式 */
	float Damage = TypeDamage + FMath::RandRange(MinAttack, MaxAttack) * AttackBonus - Defense;
	Damage = FMath::Max<float>(0.0f,Damage); //伤害最小为0, 否则会出现负数
	Damage = bIsCriticalHit ? Damage * CriticalHitDamage : Damage;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT(" %s 对 %s 造成伤害: %f | 技能等级: %d"),*SourceAvatarActor->GetName(),*TargetAvatarActor->GetName(),Damage, AbilityLevel));

	/** 修改MetaDamage属性 */
	const FGameplayModifierEvaluatedData EvaluatedData(UMageAttributeSet::GetMetaDamageAttribute(), EGameplayModOp::Additive, Damage);

	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
