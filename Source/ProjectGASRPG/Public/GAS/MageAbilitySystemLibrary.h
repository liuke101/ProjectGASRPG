// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MageAbilitySystemLibrary.generated.h"

enum class ECharacterClass : uint8;
struct FGameplayTag;
struct FGameplayEffectContextHandle;
class UCharacterClassDataAsset;
class UGameplayEffect;
class UAbilitySystemComponent;
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
	/** 授予角色GA(在CharacterClassDataAsset中设置GA)
	 * 仅可在服务器调用(目前仅敌人类调用)
	 *
	 * 原名：GiveStartupAbilities
	 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GamePlayAbility")
	static void GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass);


#pragma endregion
	
#pragma region GameplayTag
	/** 根据AbilityTag, 获取GA等级 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayTag")
	static int32 GetAbilityLevelFromTag(UAbilitySystemComponent* ASC,const FGameplayTag AbilityTag);

	/** 根据Tag判断是否是友方 */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayTag")
	static bool IsFriendly(AActor* FirstActor, AActor* SecondActor);
#pragma region endregion
	
#pragma region DataAsset
	/** 使用GE初始化默认属性, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static void InitDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, const int32 Level, UAbilitySystemComponent* ASC);
	
	/** 获取CharacterClassDataAsset, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static UCharacterClassDataAsset* GetCharacterClassDataAsset(const UObject* WorldContextObject);
#pragma endregion

#pragma region Combat
	/** 获取指定半径内的所有活着的Player */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|Combat")
	static void GetLivePlayerWithInRadius(const UObject* WorldContextObject,TArray<AActor*>& OutOverlappingActors,const TArray<AActor*>& IgnoreActors,const FVector& SphereOrigin, const float Radius);
#pragma endregion 

};
