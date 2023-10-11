#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
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

USTRUCT()
struct FEffectProperty
{
	GENERATED_BODY()

	FEffectProperty() {}
	FEffectProperty(FGameplayEffectContextHandle InEffectContextHandle, UAbilitySystemComponent* InSourceASC, AActor* InSourceAvatarActor, AController* InSourceController, ACharacter* InSourceCharacter, UAbilitySystemComponent* InTargetASC, AActor* InTargetAvatarActor, AController* InTargetController, ACharacter* InTargetCharacter)
		: EffectContextHandle(InEffectContextHandle)
		, SourceASC(InSourceASC)
		, SourceAvatarActor(InSourceAvatarActor)
		, SourceController(InSourceController)
		, SourceCharacter(InSourceCharacter)
		, TargetASC(InTargetASC)
		, TargetAvatarActor(InTargetAvatarActor)
		, TargetController(InTargetController)
		, TargetCharacter(InTargetCharacter)
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
	UAbilitySystemComponent* TargetASC = nullptr;
	UPROPERTY()
	AActor* TargetAvatarActor= nullptr;
	UPROPERTY()
	AController* TargetController= nullptr;
	UPROPERTY()
	ACharacter* TargetCharacter= nullptr;
	
};

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

	/** 仅在 (Instant)GameplayEffect 对 Attribute 的 BaseValue 修改之后触发 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** Primary Attributes */
#pragma region "生命值 Health"
	/* 生命值 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Mage_Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Mage_Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxHealth)
	
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;
#pragma endregion
#pragma region "法力值 Mana"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Mage_Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Mana)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Mage_Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxMana)
	
	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldMana) const;

	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const;
#pragma endregion
#pragma region "力量 Strength"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Mage_Attributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Strength)
	
	UFUNCTION()
	virtual void OnRep_Strength(const FGameplayAttributeData& OldStrength) const;
#pragma endregion
#pragma region "智力 Intelligence"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Mage_Attributes")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Intelligence)
	
	UFUNCTION()
	virtual void OnRep_Intelligence(const FGameplayAttributeData& OldWisdom) const;
#pragma endregion
#pragma region "防御力 Defense"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category = "Mage_Attributes")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Defense)
	
	UFUNCTION()
	virtual void OnRep_Defense(const FGameplayAttributeData& OldDefense) const;
#pragma endregion
#pragma region "精力 Vigor"
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Mage_Attributes")
	FGameplayAttributeData Vigor;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Vigor)
	
	UFUNCTION()
	virtual void OnRep_Vigor(const FGameplayAttributeData& OldVigor) const;
#pragma endregion

private:
	void SetEffectProperty(FEffectProperty& Property, const FGameplayEffectModCallbackData& Data) const; 
};
