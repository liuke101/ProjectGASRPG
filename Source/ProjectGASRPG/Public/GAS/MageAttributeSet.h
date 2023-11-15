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

	/** 属性复制列表 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Clamp属性, Attribute 的 CurrentValue 被修改前触发 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** 仅在 (Instant) GameplayEffect 对 Attribute 的 BaseValue 修改之后触发 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

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
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Health)
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "法力值 Mana"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Mana)
	
	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "活力值 Vitality"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Vitality, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData Vitality;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Vitality)
	
	UFUNCTION()
	virtual void OnRep_Vitality(const FGameplayAttributeData& OldData) const;
#pragma endregion

	/** Primary Attributes */
#pragma region "力量 Strength：总物理攻击1.3+ 最大生命值2"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Strength)
	
	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldData) const;
#pragma endregion
	
#pragma region "智力 Intelligence：总魔法攻击2.2 + 最大法力值3"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Intelligence)
	
	UFUNCTION()
	virtual void OnRep_Intelligence(const FGameplayAttributeData& OldData) const;
#pragma endregion
	
#pragma region "体力 Stamina：防御力4.8 + 最大生命值19.4"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Stamina)
	
	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "精力 Vigor：暴击率0.05 + 最小攻击1.7，最大攻击2.5 "
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Vigor, Category = "Mage_Attributes|Primary")
	FGameplayAttributeData Vigor;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Vigor)
	
	UFUNCTION()
	virtual void OnRep_Vigor(const FGameplayAttributeData& OldData) const;
#pragma endregion

	/** Secondary Attributes */
#pragma region "最大生命值 MaxHealth"

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxHealth)

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldData) const;
#pragma endregion
	
#pragma region "最大法力值 MaxMana"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxMana)

	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "最大活力值 Vitality"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxVitality, Category = "Mage_Attributes|Vital")
	FGameplayAttributeData MaxVitality;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxVitality)
	
	UFUNCTION()
	virtual void OnRep_MaxVitality(const FGameplayAttributeData& OldData) const;
#pragma endregion
	
#pragma region "物理攻击 PhysicalAttack"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxPhysicalAttack, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxPhysicalAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxPhysicalAttack)
	
	UFUNCTION()
	virtual void OnRep_MaxPhysicalAttack(const FGameplayAttributeData& OldData) const;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MinPhysicalAttack, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MinPhysicalAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MinPhysicalAttack)
	
	UFUNCTION()
	virtual void OnRep_MinPhysicalAttack(const FGameplayAttributeData& OldData) const;
#pragma endregion
	
#pragma region "魔法攻击 MagicAttack"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMagicAttack, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MaxMagicAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxMagicAttack)
	
	UFUNCTION()
	virtual void OnRep_MaxMagicAttack(const FGameplayAttributeData& OldData) const;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MinMagicAttack, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData MinMagicAttack;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MinMagicAttack)
	
	UFUNCTION()
	virtual void OnRep_MinMagicAttack(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "防御力 Defense"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Defense)
	
	UFUNCTION()
	virtual void OnRep_Defense(const FGameplayAttributeData& OldData) const;
#pragma endregion
	
#pragma region "暴击率 CriticalHitChance"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitChance, Category = "Mage_Attributes|Secondary")
	FGameplayAttributeData CriticalHitChance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, CriticalHitChance)
	
	UFUNCTION()
	virtual void OnRep_CriticalHitChance(const FGameplayAttributeData& OldData) const;
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

	/** Resistance Attributes */
#pragma region "火抗性 FireResistance"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireResistance, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, FireResistance)
	
	UFUNCTION()
	virtual void OnRep_FireResistance(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "冰抗性 IceResistance"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IceResistance, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData IceResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, IceResistance)

	UFUNCTION()
	virtual void OnRep_IceResistance(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "闪电抗性 LightningResistance"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LightningResistance, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, LightningResistance)

	UFUNCTION()
	virtual void OnRep_LightningResistance(const FGameplayAttributeData& OldData) const;
#pragma endregion

#pragma region "物理抗性 PhysicalResistance"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalResistance, Category = "Mage_Attributes|Resistance")
	FGameplayAttributeData PhysicalResistance;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, PhysicalResistance)

	UFUNCTION()
	virtual void OnRep_PhysicalResistance(const FGameplayAttributeData& OldData) const;
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
