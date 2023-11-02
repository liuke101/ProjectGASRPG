#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

/** 与GameplayTag关联的Montage */
USTRUCT(BlueprintType)
struct FTaggedMontage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly)
	FGameplayTag MontageTag = FGameplayTag::EmptyTag;
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FVector GetWeaponSocketLocation();

	/** MotionWarping 根据目标位置更新朝向 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateFacingTarget(const FVector& TargetLocation);

	/** 死亡反馈 */
	virtual void Die() = 0;

	/** 是否死亡 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsDead() const;

	/** 获取Avatar */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	const AActor* GetAvatar() const;

#pragma region Montage
	/** 获取受击反馈Montage */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UAnimMontage* GetHitReactMontage() const;

	/** 获取成员为FTaggedMontage结构体的数组 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	TArray<FTaggedMontage> GetAttackMontages() const;
#pragma endregion
	
};
