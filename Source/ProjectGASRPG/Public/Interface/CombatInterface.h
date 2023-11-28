#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAscRegisteredDelegate, UAbilitySystemComponent*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathDelegate, AActor*, DeadActor);

class UNiagaraSystem;
/** 与GameplayTag关联的Montage */
USTRUCT(BlueprintType)
struct FTaggedMontage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	FGameplayTag MontageTag = FGameplayTag::EmptyTag;  //负责触发GA Montage事件(蒙太奇设置AnimNotify -> GA WaitGameplayEvent)

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	FGameplayTag AttackSocketTag = FGameplayTag::EmptyTag; //负责获取Socket位置

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	USoundBase* ImpactSound = nullptr;
};


/** 战斗接口 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTGASRPG_API ICombatInterface
{
	GENERATED_BODY()

public:
	virtual int32 GetCharacterLevel() const = 0;
	virtual ECharacterClass GetCharacterClass() const = 0;

	/** 在接口中大量使用了BlueprintNativeEvent，这样的好处是可以使用BlueprintCallable在蓝图中调用函数, 接口函数不能直接使用BlueprintCallable */

	/** 获取武器 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	USkeletalMeshComponent* GetWeapon();
	
	/** 获取武器Socket位置 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	FVector GetWeaponSocketLocationByTag(const FGameplayTag& SocketTag) const;

	/** MotionWarping 根据目标位置更新朝向 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	void UpdateFacingTarget(const FVector& TargetLocation);

	/** 死亡反馈 */
	virtual void Die(const FVector& DeathImpulse) = 0;

	/** 是否死亡 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	bool IsDead() const;

	/** 获取Avatar */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	const AActor* GetAvatar() const;

#pragma region GAS
	/** 获取召唤物数量 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface|GAS")
	int32 GetSummonCount() const;

	/** 修改召唤物计数（加减） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface|GAS")
	void ModifySummonCount(const int32 Count);
	
#pragma endregion
	
#pragma region Montage
	/** 获取受击反馈Montage */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface|Montage")
	UAnimMontage* GetHitReactMontage() const;
	/** 获取成员为FTaggedMontage结构体的数组 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface|Montage")
	TArray<FTaggedMontage> GetAttackMontages() const;

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = "Mage_CombatInterface|Montage")
	FTaggedMontage GetRandomAttackMontage() const;

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = "Mage_CombatInterface|Montage")
	FTaggedMontage GetTaggedMontageByTag(const FGameplayTag& MontageTag) const;

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = "Mage_CombatInterface|Montage")
	void SetInCastingLoop(bool bInCastingLoop);
#pragma endregion

#pragma region Effect
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface|Effect")
	UNiagaraSystem* GetHitEffect() const;
#pragma endregion

	/** 获取ASC注册委托, 注意返回值为引用类型 */
	virtual FOnAscRegisteredDelegate& GetOnASCRegisteredDelegate() = 0;

	/** 获取OnDeath委托, 注意返回值为引用类型 */
	virtual FOnDeathDelegate& GetOnDeathDelegate() = 0;

};
