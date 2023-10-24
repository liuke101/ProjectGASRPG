// 

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MageAbilitySystemLibrary.generated.h"

class UCharacterClassDataAsset;
class UGameplayEffect;
class UAbilitySystemComponent;
enum class ECharacterClass : uint8;
class UAttributeMenuWidgetController;
class UOverlayWidgetController;
/**
 *  GAS蓝图库
 */
UCLASS()
class PROJECTGASRPG_API UMageAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 获取OverlayWidgetController */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController")
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);

	/** 获取AttributeMenuWidgetController */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController")
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);

	/** 使用GE初始化默认属性 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|CharacterClassInfo")
	static void InitDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, const int32 Level, UAbilitySystemComponent* ASC);

	/** ApplyGameplayEffectSpecToSelf */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffect")
	static void ApplyEffectToSelf(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> GameplayEffectClass,const float Level);

	/** 授予角色GA */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GamePlayAbility")
	static void GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC);

	/** 获取CharacterClassDataAsset */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static UCharacterClassDataAsset * GetCharacterClassDataAsset(const UObject* WorldContextObject);

	
};
