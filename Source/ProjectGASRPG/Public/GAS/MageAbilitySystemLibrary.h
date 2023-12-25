#pragma once
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "MageAbilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MageAbilitySystemLibrary.generated.h"

class UInventoryComponent;
class UEquipmentWidgetController;
struct FScalableFloat;

UENUM()
enum class EExecNodePin : uint8
{
	Layer1,
	Layer2,
	Layer3,
	Layer4,
};

UENUM()
enum class EColliderShape : uint8
{
	Sphere,
	Box,
	Capsule,
};


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
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|WidgetController")
	static FWidgetControllerParams MakeWidgetControllerParams(APlayerController* PC);

	/** 获取OverlayWidgetController */
	// meta = (DefaultToSelf = "WorldContextObject")默认将WorldContextObject参数设置为self
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);

	/** 获取AttributeMenuWidgetController */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);

	/** 获取SkillTreeWidgetController */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static USkillTreeWidgetController* GetSkillTreeWidgetController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UEquipmentWidgetController * GetEquipmentWidgetController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|WidgetController",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UInventoryWidgetController * GetInventoryWidgetController(const UObject* WorldContextObject);
#pragma endregion

#pragma region GameplayEffect
	/** ApplyGameplayEffectSpecToSelf */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffect")
	static void ApplyEffectToSelf(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> GameplayEffectClass, const float Level);

	/** 应用Damage GE */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffect")
	static FGameplayEffectContextHandle ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams);
#pragma endregion

#pragma region FMageGameplayEffectContext
	/** 获取爆击状态 */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static bool GetIsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle);

	/**
	 * 设置暴击状态
	 * - UPARAM(ref)宏：使参数通过非const引用传递并仍然显示为输入引脚（默认为输出）
	 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetIsCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const bool bIsCriticalHit);
	
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static bool GetIsDebuff(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetIsDebuff(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const bool bIsDebuff);

	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static float GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetDebuffDamage(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const float DebuffDamage);
	
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static float GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetDebuffFrequency(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const float DebuffFrequency);
	
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static float GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetDebuffDuration(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const float DebuffDuration);
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static FGameplayTag GetDamageTypeTag(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetDamageTypeTag(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FGameplayTag& DamageTypeTag);

	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static FVector GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetDeathImpulse(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FVector DeathImpulse);

	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static FVector GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle);
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayEffectContext")
	static void SetKnockbackForce(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FVector KnockbackForce);
#pragma endregion

#pragma region GameplayAbility
	/** 授予角色GA(在CharacterClassDataAsset中设置GA) */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayAbility",meta = (DefaultToSelf = "WorldContextObject"))
	static void GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC,  ECharacterClass CharacterClass);

	/** 读取ScalableFloat, 蓝图中没有默认接口 */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayAbility")
	static float GetScalableFloatValueAtLevel(const FScalableFloat& ScalableFloat, const int32 Level);
#pragma endregion

#pragma region GameplayCue
	/** 根据蓄力时间获取层数 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayCue", meta=(ExpandEnumAsExecs="Result"))
	static void GetAbilityLayerByTimeHeld(EExecNodePin& Result, const float TimeHeld);
#pragma endregion

#pragma region GameplayTag
	/** 根据AbilityTag, 获取GA等级 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|GameplayTag")
	static int32 GetAbilityLevelFromTag(UAbilitySystemComponent* ASC, const FGameplayTag AbilityTag);

	/** 根据Tag判断是否是友方 */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|GameplayTag")
	static bool IsFriendly(AActor* FirstActor, AActor* SecondActor);
#pragma region endregion

#pragma region DataAsset
	/** 使用GE初始化默认属性, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|DataAsset",meta = (DefaultToSelf = "WorldContextObject"))
	static void InitDefaultAttributesByCharacterClass(const UObject* WorldContextObject, ECharacterClass CharacterClass, const int32 Level, UAbilitySystemComponent* ASC);

	/** 获取CharacterClassDataAsset, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|DataAsset",meta = (DefaultToSelf = "WorldContextObject"))
	static UCharacterClassDataAsset* GetCharacterClassDataAsset(const UObject* WorldContextObject);

	/** 获取CharacterClassDataAsset, 仅可在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|DataAsset",meta = (DefaultToSelf = "WorldContextObject"))
	static UAbilityDataAsset* GetAbilityDataAsset(const UObject* WorldContextObject);

	/** 根据敌人类型和等级获取经验值奖励 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|DataAsset",meta = (DefaultToSelf = "WorldContextObject"))
	static int32 GetExpRewardForClassAndLevel(const UObject* WorldContextObject, ECharacterClass CharacterClass,
	                                          const int32 CharacterLevel);
#pragma endregion

#pragma region Combat
	/** 获取指定碰撞体形状的HitResults */
	UFUNCTION(Category = "MageAbilitySystemLibrary|Combat",meta = (DefaultToSelf = "WorldContextObject"))
	static void GetOverlapResultsInCollisionShape(const UObject* WorldContextObject, TArray<FOverlapResult>& OverlapResults,const TArray<AActor*>& IgnoreActors, const FVector& Origin, const EColliderShape ColliderShape, const bool Debug = false, const float SphereRadius = 0, const FVector BoxHalfExtent = FVector(0), const float CapsuleRadius = 0, const float CapsuleHalfHeight = 0);
	
	/** 获取指定碰撞体形状内的指定类Actor */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|Combat",meta = (DefaultToSelf = "WorldContextObject"))
	static void GetActorInCollisionShapeWithClass(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors,TSubclassOf<AActor> ActorClass, const TArray<AActor*>& IgnoreActors, const FVector& Origin, const EColliderShape ColliderShape, const bool Debug = false, const float SphereRadius = 0, const FVector BoxHalfExtent = FVector(0), const float CapsuleRadius = 0, const float CapsuleHalfHeight = 0);

	/** 获取指定碰撞体形状内的所有活着的Player */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|Combat",meta = (DefaultToSelf = "WorldContextObject"))
	static void GetLivingActorInCollisionShape(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors,const TArray<AActor*>& IgnoreActors, const FVector& Origin, const EColliderShape ColliderShape, const bool Debug = false, const float SphereRadius = 0, const FVector BoxHalfExtent = FVector(0), const float CapsuleRadius = 0, const float CapsuleHalfHeight = 0);

	/** 获取指定碰撞体形状内的所有活着的敌人 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|Combat",meta = (DefaultToSelf = "WorldContextObject"))
	static void GetLivingEnemyInCollisionShape(const UObject* WorldContextObject,AActor* OwnerActor, TArray<AActor*>& OutOverlappingActors,const TArray<AActor*>& IgnoreActors, const FVector& Origin, const EColliderShape ColliderShape, const bool Debug = false, const float SphereRadius = 0, const FVector BoxHalfExtent = FVector(0), const float CapsuleRadius = 0, const float CapsuleHalfHeight = 0);

	/** 获取距离 Origin 最近的Actor数组 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|Combat")
	static AActor* GetClosestActor(const TArray<AActor*>& CheckedActors, const FVector& Origin);
	
	/** 获取距离 Origin 最近的Actor数组 */
	UFUNCTION(BlueprintCallable, Category = "MageAbilitySystemLibrary|Combat")
	static void GetClosestActors(const TArray<AActor*>& CheckedActors, TArray<AActor*>& OutClosestActors,const FVector& Origin, const int32 MaxTargetNum);

	/** 获取TargettingActor */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|Combat",meta = (DefaultToSelf = "WorldContextObject"))
	static AActor* GetTargetingActor(const UObject* WorldContextObject);

	/** 获取MagicCircleLocation */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|Combat",meta = (DefaultToSelf = "WorldContextObject"))
	static FVector GetMagicCircleLocation(const UObject* WorldContextObject);
#pragma endregion

#pragma region Math
	/** 返回按角度均匀分布的Rotator */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|Math")
	static TArray<FRotator> EvenlySpacedRotators(const FVector& Forward, const FVector& Axis,const float SpreadAngle, const int32 SpreadNum);
	
	/** 返回按角度均匀分布的Vector */
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|Math")
	static TArray<FVector> EvenlySpacedVectors(const FVector& Forward, const FVector& Axis,const float SpreadAngle, const int32 SpreadNum);
#pragma endregion

#pragma region Inventory
	UFUNCTION(BlueprintPure, Category = "MageAbilitySystemLibrary|Inventory",
		meta = (DefaultToSelf = "WorldContextObject"))
	static UInventoryComponent* GetInventoryComponent(const UObject* WorldContextObject);

#pragma endregion	
};
