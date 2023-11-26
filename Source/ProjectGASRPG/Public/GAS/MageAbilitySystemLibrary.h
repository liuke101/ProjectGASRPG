// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "MageAbilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MageAbilitySystemLibrary.generated.h"

class UAbilityDataAsset;
class USkillTreeWidgetController;
enum class ECharacterClass : uint8;
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
	/** 创建WidgetControllerParams */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController")
	static FWidgetControllerParams MakeWidgetControllerParams(APlayerController* PC);

	/** 获取OverlayWidgetController */
	// meta = (DefaultToSelf = "WorldContextObject")默认将WorldContextObject参数设置为self
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);

	/** 获取AttributeMenuWidgetController */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);

	/** 获取SkillTreeWidgetController */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static USkillTreeWidgetController* GetSkillTreeWidgetController(const UObject* WorldContextObject);
#pragma endregion

#pragma region GameplayEffect
	/** ApplyGameplayEffectSpecToSelf */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffect")
	static void ApplyEffectToSelf(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> GameplayEffectClass,const float Level);

	/** 应用Damage GE */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffect")
	static FGameplayEffectContextHandle ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams);
#pragma endregion

#pragma region  FMageGameplayEffectContext
	/** 获取爆击状态 */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static bool GetIsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle);

	/**
	 * 设置暴击状态
	 * - UPARAM(ref)宏：使参数通过非const引用传递并仍然显示为输入引脚（默认为输出）
	 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const bool bIsCriticalHit);
	
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static bool GetIsDebuff(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetIsDebuff(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const bool bIsDebuff);

	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static float GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetDebuffDamage(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const float DebuffDamage);
	
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static float GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetDebuffFrequency(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const float DebuffFrequency);
	
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static float GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetDebuffDuration(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const float DebuffDuration);
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static FGameplayTag GetDamageTypeTag(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetDamageTypeTag(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FGameplayTag& DamageTypeTag);

	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static FVector GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetDeathImpulse(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FVector DeathImpulse);

	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static FVector GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayEffectContext")
	static void SetKnockbackForce(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FVector KnockbackForce);
#pragma endregion

#pragma region GameplayAbility
	/** 授予角色GA(在CharacterClassDataAsset中设置GA)
	 * 仅可在服务器调用(目前仅敌人类调用)
	 *
	 * 原名：GiveStartupAbilities
	 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayAbility")
	static void GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC,
	                                   ECharacterClass CharacterClass);

	/** 读取ScalableFloat, 蓝图中没有默认接口 */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayAbility")
	static float GetScalableFloatValueAtLevel(const FScalableFloat& ScalableFloat, const int32 Level);
#pragma endregion

#pragma region GameplayTag
	/** 根据AbilityTag, 获取GA等级 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|GameplayTag")
	static int32 GetAbilityLevelFromTag(UAbilitySystemComponent* ASC, const FGameplayTag AbilityTag);

	/** 根据Tag判断是否是友方 */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|GameplayTag")
	static bool IsFriendly(AActor* FirstActor, AActor* SecondActor);
#pragma region endregion

#pragma region DataAsset
	/** 使用GE初始化默认属性, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static void InitDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass,
	                                  const int32 Level, UAbilitySystemComponent* ASC);

	/** 获取CharacterClassDataAsset, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static UCharacterClassDataAsset* GetCharacterClassDataAsset(const UObject* WorldContextObject);

	/** 获取CharacterClassDataAsset, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static UAbilityDataAsset* GetAbilityDataAsset(const UObject* WorldContextObject);

	/** 根据敌人类型和等级获取经验值奖励 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|DataAsset")
	static int32 GetExpRewardForClassAndLevel(const UObject* WorldContextObject, ECharacterClass CharacterClass,
	                                          const int32 CharacterLevel);
#pragma endregion

#pragma region Combat
	/** 获取指定半径内的所有活着的Player */
	UFUNCTION(BlueprintCallable, Category = "Mage_AbilitySystemBPLibrary|Combat")
	static void GetLivePlayerWithInRadius(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors,
	                                      const TArray<AActor*>& IgnoreActors, const FVector& SphereOrigin,
	                                      const float Radius);
#pragma endregion

#pragma region Math
	/** 返回按角度均匀分布的Rotator */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|Math")
	static TArray<FRotator> EvenlySpacedRotators(const FVector& Forward, const FVector& Axis,const float SpreadAngle, const int32 SpreadNum);
	
	/** 返回按角度均匀分布的Vector */
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|Math")
	static TArray<FVector> EvenlySpacedVectors(const FVector& Forward, const FVector& Axis,const float SpreadAngle, const int32 SpreadNum);
#pragma endregion
};
