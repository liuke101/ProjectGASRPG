// 

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MageAbilitySystemLibrary.generated.h"

struct FGameplayTag;
struct FGameplayEffectContextHandle;
class UCharacterClassDataAsset;
class UGameplayEffect;
class UAbilitySystemComponent;
enum class ECharacterClass : uint8;
class UAttributeMenuWidgetController;
class UOverlayWidgetController;

/** GAS函数库 */
UCLASS()
class PROJECTGASRPG_API UMageAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#pragma region WidgetController
	/** 获取OverlayWidgetController */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController")
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);

	/** 获取AttributeMenuWidgetController */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController")
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);
#pragma endregion

#pragma region GameplayEffect
	/** ApplyGameplayEffectSpecToSelf */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffect")
	static void ApplyEffectToSelf(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> GameplayEffectClass,const float Level);

	/** 获取爆击状态 */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffect")
	static bool GetIsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle);

	/** 设置暴击状态 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffect")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, bool bIsCriticalHit);
	//UPARAM(ref)宏：使参数通过非const引用传递并仍然显示为输入引脚（默认为输出）
#pragma endregion

#pragma region GameplayAbility
	/** 授予角色GA, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GamePlayAbility")
	static void GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC);

	/** 根据AbilityTag, 获取GA等级 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GamePlayAbility")
	static int32 GetAbilityLevelFromTag(UAbilitySystemComponent* ASC,const FGameplayTag AbilityTag);
#pragma endregion

#pragma region DataAsset
	/** 使用GE初始化默认属性, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static void InitDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, const int32 Level, UAbilitySystemComponent* ASC);
	
	/** 获取CharacterClassDataAsset, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static UCharacterClassDataAsset* GetCharacterClassDataAsset(const UObject* WorldContextObject);
#pragma endregion

#pragma region GameplayTag
	/** 获取所有匹配 GameplayTag 的 Actor（Actor应该有GameplayTagsComponent） */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayTag")
	static void GetAllActorsWithGameplayTag(const UObject* WorldContextObject,const  FGameplayTag& InGameplayTag, TArray<AActor*>& OutActors, const bool bIsExact = true);

#pragma endregion
};
