#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MageAttributeSet.generated.h"

/*
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

/* 属性集可以有多个，本项目只使用一个*/
UCLASS()
class PROJECTGASRPG_API UMageAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMageAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	/* 生命值 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Mage_Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Mage_Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxHealth)

	/* 魔法值 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category = "Mage_Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category = "Mage_Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UMageAttributeSet, MaxMana)

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldMana) const;

	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const;
};
