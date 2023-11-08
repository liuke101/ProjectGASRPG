﻿#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

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
	FGameplayTag SocketTag = FGameplayTag::EmptyTag; //负责获取Socket位置

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

	/** 获取武器Socket位置 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	FVector GetWeaponSocketLocationByTag(const FGameplayTag& SocketTag) const;

	/** MotionWarping 根据目标位置更新朝向 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	void UpdateFacingTarget(const FVector& TargetLocation);

	/** 死亡反馈 */
	virtual void Die() = 0;

	/** 是否死亡 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	bool IsDead() const;

	/** 获取Avatar */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	const AActor* GetAvatar() const;

#pragma region Montage
	/** 获取受击反馈Montage */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	UAnimMontage* GetHitReactMontage() const;
	/** 获取成员为FTaggedMontage结构体的数组 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	TArray<FTaggedMontage> GetAttackMontages() const;

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = "Mage_CombatInterface")
	FTaggedMontage GetRandomAttackMontage() const;

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = "Mage_CombatInterface")
	FTaggedMontage GetTaggedMontageByTag(const FGameplayTag& MontageTag) const;
#pragma endregion

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_CombatInterface")
	UNiagaraSystem* GetHitEffect() const;
	
};
