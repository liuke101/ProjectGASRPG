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
	}
};

/** MageDamageStatics 单例 */
// 静态函数与普通函数不同，它只能在声明它的文件当中可见，不能被其它文件使用。
static const MageDamageStatics& DamageStatics()
{
	//C++11 局部静态变量只会初始化一次
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
	/** 构造 AttributeTag 与 CaptureDef 的映射 */
	TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition> AttributeTag_To_CaptureDef;
	MakeAttributeTag_To_CaptureDef(AttributeTag_To_CaptureDef);

	/** 获取ASC */
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	/** 获取AvatarActor */
	AActor* SourceAvatarActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	AActor* TargetAvatarActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	/** 获取GameplayEffectSpec */
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

	/** 获取GameplayEffectContext */
	FGameplayEffectContextHandle EffectContextHandle = EffectSpec.GetContext();
	
	/** 获取Tag */
	const FGameplayTagContainer* SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	/** 捕获属性 */
	float Source_MinAttack= 0.0f;
	float Source_MaxAttack = 0.0f;
	float Source_CriticalHitChance = 0.0f; //暴击率
	float Target_Defense = 0.0f; //防御力
	
	// 法师捕获法术攻击力，其他捕获物理攻击力 
	if(Cast<ICombatInterface>(SourceAvatarActor)->GetCharacterClass() == ECharacterClass::Mage)
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MinMagicAttackDef, EvaluationParameters, Source_MinAttack);
		Source_MinAttack = FMath::Max<float>(0.0f,Source_MinAttack);
	
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxMagicAttackDef, EvaluationParameters, Source_MaxAttack);
		Source_MaxAttack = FMath::Max<float>(0.0f,Source_MaxAttack);
	}
	else
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MinPhysicalAttackDef, EvaluationParameters, Source_MinAttack);
		Source_MinAttack = FMath::Max<float>(0.0f,Source_MinAttack);
	
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().MaxPhysicalAttackDef, EvaluationParameters, Source_MaxAttack);
		Source_MaxAttack = FMath::Max<float>(0.0f,Source_MaxAttack);
	}
	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalHitChanceDef, EvaluationParameters, Source_CriticalHitChance);
	Source_CriticalHitChance = FMath::Max<float>(0.0f,Source_CriticalHitChance);
	
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluationParameters, Target_Defense);
	Target_Defense = FMath::Max<float>(0.0f,Target_Defense);

	
	/** 计算Debuff */
	CalcDebuff(ExecutionParams, EvaluationParameters, EffectSpec, EffectContextHandle, AttributeTag_To_CaptureDef);
	
	/*
	 * TypeDamage
	 * Set By Caller 获取技能类型伤害，在GA中设置
	 */
	float Source_TypeDamage = 0;
	// 遍历所有伤害类型
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
	for(auto& Pair : MageGameplayTags.DamageTypeTag_To_ResistanceTag)
	{
		FGameplayTag DamageTypeTag = Pair.Key;
		FGameplayTag ResistanceTag = Pair.Value;
		
		/**
		 * 获取伤害类型对应的magnitude (在GA中通过AssignTagSetByCallerMagnitude设置)
		 * 如果伤害为0, 跳过该类型的计算(当GA中的DamageTypeTag_To_AbilityDamage映射没有设置对应的伤害类型时，自然无法通过SetByCaller获取magnitude,GetSetByCallerMagnitude会返回0)
		 */
		float Source_TypeDamageMagnitude = EffectSpec.GetSetByCallerMagnitude(DamageTypeTag,false, 0); //找不到就返回0, 指明不报错
		if(Source_TypeDamageMagnitude == 0) continue; 

		checkf(AttributeTag_To_CaptureDef.Contains(ResistanceTag), TEXT("抗性Tag %s 没有对应的捕获属性定义"), *ResistanceTag.ToString());
	    const FGameplayEffectAttributeCaptureDefinition CaptureDef = AttributeTag_To_CaptureDef[ResistanceTag]; //获取抗性Tag对应的捕获属性定义

		// 获取Target抗性属性
		float Target_TypeResistance = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, Target_TypeResistance);
		Target_TypeResistance = FMath::Clamp<float>(Target_TypeResistance, 0.0f,1.0f);

		// 计算最终的类型伤害
		// TODO:这里用+=会叠加所有类型的伤害，如果只想计算一种类型的伤害，可以用=，具体是否会有bug，等遇到多属性伤害的时候再测试
		Source_TypeDamage += Source_TypeDamageMagnitude * (1 - Target_TypeResistance);
	}

	/** 获得技能等级 */
	const int32 AbilityLevel = EffectSpec.GetLevel();
	
	/** 根据技能等级读取曲线表格，获取攻击加成和暴击伤害数据 */
	const UCharacterClassDataAsset* CharacterClassDataAsset = UMageAbilitySystemLibrary::GetCharacterClassDataAsset(SourceAvatarActor);
	const float AttackBonus = CharacterClassDataAsset->CalcDamageCurveTable->FindCurve(FName(TEXT("ExecCalc.AttackBonus")),FString(),true)->Eval(AbilityLevel);
	const float CriticalHitDamage = CharacterClassDataAsset->CalcDamageCurveTable->FindCurve(FName(TEXT("ExecCalc.CriticalHitDamage")),FString(),true)->Eval(AbilityLevel);
	
	/** 是否暴击 */
	const bool bIsCriticalHit = FMath::RandRange(0.f, 1.f) <= Source_CriticalHitChance;

	/** 获取EffectContext，同步暴击状态 */
	UMageAbilitySystemLibrary::SetIsCriticalHit(EffectContextHandle, bIsCriticalHit);

	/** 伤害公式 */
	float FinalDamage = Source_TypeDamage + FMath::RandRange(Source_MinAttack, Source_MaxAttack) * AttackBonus - Target_Defense;
	FinalDamage = FMath::Max<float>(0.0f,FinalDamage); //伤害最小为0, 否则会出现负数
	FinalDamage = bIsCriticalHit ? FinalDamage * CriticalHitDamage : FinalDamage;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT(" %s 对 %s 造成伤害: %f | 技能等级: %d"),*SourceAvatarActor->GetName(),*TargetAvatarActor->GetName(),FinalDamage, AbilityLevel));

	/** 修改MetaDamage属性 */
	const FGameplayModifierEvaluatedData EvaluatedData(UMageAttributeSet::GetMetaDamageAttribute(), EGameplayModOp::Additive, FinalDamage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}

void UExecCalc_Damage::MakeAttributeTag_To_CaptureDef(
	TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition>& AttributeTag_To_CaptureDef)
{
	const FMageGameplayTags& Tags = FMageGameplayTags::Instance();
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_MinMagicAttack, DamageStatics().MinMagicAttackDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_MaxMagicAttack, DamageStatics().MaxMagicAttackDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_MinPhysicalAttack, DamageStatics().MinPhysicalAttackDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_MaxPhysicalAttack, DamageStatics().MaxPhysicalAttackDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_CriticalHitChance, DamageStatics().CriticalHitChanceDef);
	
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Secondary_Defense, DamageStatics().DefenseDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Fire, DamageStatics().FireResistanceDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Ice, DamageStatics().IceResistanceDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Lightning, DamageStatics().LightningResistanceDef);
	AttributeTag_To_CaptureDef.Add(Tags.Attribute_Resistance_Physical, DamageStatics().PhysicalResistanceDef);
}

void UExecCalc_Damage::CalcDebuff(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	const FAggregatorEvaluateParameters& EvaluationParameters, const FGameplayEffectSpec& EffectSpec,
	FGameplayEffectContextHandle& EffectContextHandle, const TMap<FGameplayTag, FGameplayEffectAttributeCaptureDefinition>& AttributeTag_To_CaptureDef) const
{
	//遍历所有Debuff类型
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
	for(auto& Pair:MageGameplayTags.DamageTypeTag_To_DebuffTag)
	{
		const FGameplayTag DamageTypeTag = Pair.Key;
		const FGameplayTag DebuffTag = Pair.Value;

		// SetByCaller 获取TypeDamage, 找不到就返回0(说明该属性无效)
		const float TypeDamageMagnitude = EffectSpec.GetSetByCallerMagnitude(DamageTypeTag,false, 0); 
		if(TypeDamageMagnitude == 0) continue;
		
		// SetByCaller 获取Debuff几率, 找不到就返回0
		const float SourceDebuffChance = EffectSpec.GetSetByCallerMagnitude(MageGameplayTags.Debuff_Params_Chance,false, 0); 
		if(SourceDebuffChance == 0) continue;
		
		// 获取Target抗性属性(作为Debuff抗性)
		float TargetTypeResistance = 0.0f;
		const FGameplayTag& ResistanceTag = MageGameplayTags.DamageTypeTag_To_ResistanceTag[DamageTypeTag];
		const FGameplayEffectAttributeCaptureDefinition CaptureDef = AttributeTag_To_CaptureDef[ResistanceTag]; // 获取抗性Tag对应的捕获属性定义
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, EvaluationParameters, TargetTypeResistance);
		TargetTypeResistance = FMath::Clamp<float>(TargetTypeResistance, 0.0f,1.0f);

		// 计算最终的Debuff几率
		const float DebuffChance = SourceDebuffChance * (1 - TargetTypeResistance);

		//是否触发Debuff
		const bool bDebuff = FMath::RandRange(0.f, 1.f) <= DebuffChance;

		//触发Debuff
		if(bDebuff)
		{
			// SetByCaller 获取Debuff信息
			const float DebuffDamage = EffectSpec.GetSetByCallerMagnitude(MageGameplayTags.Debuff_Params_Damage,false, 0); 
			const float DebuffFrequency = EffectSpec.GetSetByCallerMagnitude(MageGameplayTags.Debuff_Params_Frequency,false, 0);
			const float DebuffDuration = EffectSpec.GetSetByCallerMagnitude(MageGameplayTags.Debuff_Params_Duration,false, 0);
			
			UMageAbilitySystemLibrary::SetIsDebuff(EffectContextHandle, true);
			UMageAbilitySystemLibrary::SetDamageTypeTag(EffectContextHandle, DamageTypeTag);
			UMageAbilitySystemLibrary::SetDebuffDamage(EffectContextHandle, DebuffDamage);
			UMageAbilitySystemLibrary::SetDebuffFrequency(EffectContextHandle, DebuffFrequency);
			UMageAbilitySystemLibrary::SetDebuffDuration(EffectContextHandle, DebuffDuration);
			
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("触发Debuff: %f"), DebuffChance));
		}
	}
}




