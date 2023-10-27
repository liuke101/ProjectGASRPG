#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"
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
	
	virtual FVector GetWeaponSocketLocation();

	/** MotionWarping 根据目标位置更新朝向 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateFacingTarget(const FVector& TargetLocation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UAnimMontage* GetHitReactMontage() const;

	virtual void Die() = 0;
};
