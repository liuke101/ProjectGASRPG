#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Data/CharacterClassDataAsset.h"
#include "MageAttributeSet.generated.h"

/**
 * 该宏复制自AttributeSet.h
 * 
 * 自动为每个Attribute生成以下函数：（以Health为例）
 * static FGameplayAttribute UMyHealthSet::GetHealthAttribute(); 
 * FORCEINLINE float UMyHealthSet::GetHealth() const; ==> GetCurrentValue
 * FORCEINLINE void UMyHealthSet::SetHealth(float NewVal); ==>  SetBaseValue
 * FORCEINLINE void UMyHealthSet::InitHealth(float NewVal); ==> SetBaseValue / SetCurrentValue
 *	
 * 用法：ATTRIBUTE_ACCESSORS(UMyHealthSet, Health)
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
		GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
		GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


enum class ECharacterClass : uint8;

/** 存储和Effect相关联的变量，在PostGameplayEffectExecute()不断更新 */
USTRUCT()
struct FEffectProperty
{
	GENERATED_BODY()

	FEffectProperty() {}
	FEffectProperty(const FGameplayEffectContextHandle& InEffectContextHandle, UAbilitySystemComponent* InSourceASC, AActor* InSourceAvatarActor, AController* InSourceController, ACharacter* InSourceCharacter, const int32 InSourceCharacterLevel, const ECharacterClass InSourceCharacterClass,   UAbilitySystemComponent* InTargetASC, AActor* InTargetAvatarActor, AController* InTargetController, ACharacter* InTargetCharacter,const int32 InTargetCharacterLevel, const ECharacterClass InTargetCharacterClass)
		: EffectContextHandle(InEffectContextHandle)
		, SourceASC(InSourceASC)
		, SourceAvatarActor(InSourceAvatarActor)
		, SourceController(InSourceController)
		, SourceCharacter(InSourceCharacter)
		, SourceCharacterLevel(InSourceCharacterLevel)
		, SourceCharacterClass(InSourceCharacterClass)
		, TargetASC(InTargetASC)
		, TargetAvatarActor(InTargetAvatarActor)
		, TargetController(InTargetController)
		, TargetCharacter(InTargetCharacter)
		, TargetCharacterLevel(InTargetCharacterLevel)
		, TargetCharacterClass(InTargetCharacterClass)
	{}
	
	UPROPERTY()
	FGameplayEffectContextHandle EffectContextHandle;
	
	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;
	UPROPERTY()
	AActor* SourceAvatarActor = nullptr;
	UPROPERTY()
	AController* SourceController = nullptr;
	UPROPERTY()
	ACharacter* SourceCharacter= nullptr;
	UPROPERTY()
	int32 SourceCharacterLevel = 0;
	UPROPERTY()
	ECharacterClass SourceCharacterClass = ECharacterClass::None;

	UPROPERTY()
	UAbilitySystemComponent* TargetASC = nullptr;
	UPROPERTY()
	AActor* TargetAvatarActor= nullptr;
	UPROPERTY()
	AController* TargetController= nullptr;
	UPROPERTY()
	ACharacter* TargetCharacter= nullptr;
	UPROPERTY()
	int32 TargetCharacterLevel = 0;
	UPROPERTY()
	ECharacterClass TargetCharacterClass = ECharacterClass::None;
};

/**
 * Static函数指针模板
 * 
 * 等价实现（仅无参时，TBaseStaticDelegateInstance支持可变参数）
 * template<typename T>
 * using TFuncPtr = T (*)();
 */
template<typename T>
using TStaticFuncPtr = typename TBaseStaticDelegateInstance<T,FDefaultDelegateUserPolicy>::FFuncPtr;


/** 属性集可以有多个，本项目只使用一个*/
UCLASS()
class PROJECTGASRPG_API UMageAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMageAttributeSet();

	/** Clamp属性, Attribute 的 CurrentValue 被修改前触发 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** Attribute 的 BaseValue 修改后触发 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	/** 伤害计算 */
	void CalcMetaDamage(const FEffectProperty& Property);

	/** DeBuff(使用C++创建运行时GE) */
	void Debuff(const FEffectProperty& Property);
	
	/** 获取经验值计算 */
	void CalcMetaExp(const FEffectProperty& Property);

public:
	/**
	 * 用于AttributeMenuWidgetController广播初始值
	 * 第二个参数为函数指针，返回FGameplayAttribute
	 *
	 * 等价实现：
	 * 1. typedef FGameplayAttribute (*FuncPtr)(); 或 using FuncPtr = FGameplayAttribute (*)();
	 *    TMap<FGameplayTag, FuncPtr> TagsToAttributes;
	 * 2. TMap<FGameplayTag, FGameplayAttribute (*)()> TagsToAttributes;
	 * 3. 如下，使用模板
	 */
	TMap<FGameplayTag, TStaticFuncPtr<FGameplayAttribute()>> AttributeTag_To_GetAttributeFuncPtr;

	/** Vital Attributes */
#pragma region "生命值 Health"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Health)
#pragma endregion

#pragma region "法力值 Mana"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Mana)
#pragma endregion

#pragma region "活力值 Vitality"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData Vitality;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Vitality)
#pragma endregion

#pragma region "移动速度 MoveSpeed"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MoveSpeed)

	/** Primary Attributes */
#pragma region "力量 Strength：总物理攻击1.3+ 最大生命值2"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Strength)
#pragma endregion
	
#pragma region "智力 Intelligence：总魔法攻击2.2 + 最大法力值3"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Intelligence)
#pragma endregion
	
#pragma region "体力 Stamina：防御力4.8 + 最大生命值19.4"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Stamina)
#pragma endregion

#pragma region "精力 Vigor：暴击率0.05 + 最小攻击1.7，最大攻击2.5 "
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Vigor;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Vigor)
#pragma endregion

	/** Secondary Attributes */
#pragma region "最大生命值 MaxHealth"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxHealth)
#pragma endregion
	
#pragma region "最大法力值 MaxMana"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxMana)
#pragma endregion

#pragma region "最大活力值 Vitality"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData MaxVitality;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxVitality)
#pragma endregion
	
#pragma region "物理攻击 PhysicalAttack"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxPhysicalAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxPhysicalAttack)

	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MinPhysicalAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MinPhysicalAttack)
#pragma endregion
	
#pragma region "魔法攻击 MagicAttack"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxMagicAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxMagicAttack)

	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MinMagicAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MinMagicAttack)
#pragma endregion

#pragma region "防御力 Defense"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Defense)
#pragma endregion
	
#pragma region "暴击率 CriticalHitChance"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData CriticalHitChance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, CriticalHitChance)
#pragma endregion
	
	/** Meta Attributes */
#pragma region "伤害值元属性 MetaDamage"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Meta")
	FGameplayAttributeData MetaDamage;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MetaDamage)
#pragma endregion

#pragma region "经验值元属性 MetaExp"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Meta")
	FGameplayAttributeData MetaExp;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MetaExp)
#pragma endregion
	/** Resistance Attributes */
#pragma region "火抗性 FireResistance"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, FireResistance)
#pragma endregion

#pragma region "冰抗性 IceResistance"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData IceResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, IceResistance)
#pragma endregion

#pragma region "闪电抗性 LightningResistance"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, LightningResistance)
#pragma endregion

#pragma region "物理抗性 PhysicalResistance"
	UPROPERTY(BlueprintReadOnly, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, PhysicalResistance)
#pragma endregion
	
private:
	/** 设置Effect相关属性 */
	void SetEffectProperty(FEffectProperty& Property, const FGameplayEffectModCallbackData& Data) const;

	/** 发送经验事件 */
	void SendExpEvent(const FEffectProperty& Property);
	
	/**  更新 Secondary Attributes */
	void UpdateMaxHealth(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateMaxMana(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateMaxVitality(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateMinPhysicalAttack(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateMaxPhysicalAttack(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateMinMagicAttack(ECharacterClass CharacterClass,float CharacterLevel);
	
	void UpdateMaxMagicAttack(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateDefense(ECharacterClass CharacterClass,float CharacterLevel);

	void UpdateCriticalHitChance(ECharacterClass CharacterClass,float CharacterLevel);
	
#pragma region UI
private:
	/** 显示伤害浮动文字 */
	void ShowDamageFloatingText(const FEffectProperty& Property, const float DamageValue, const bool bIsCriticalHit) const;
#pragma endregion
};

