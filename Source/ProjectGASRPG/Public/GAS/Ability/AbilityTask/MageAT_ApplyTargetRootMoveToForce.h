// 

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "MageAT_ApplyTargetRootMoveToForce.generated.h"

enum class ERootMotionMoveToActorTargetOffsetType : uint8;
class UCharacterMovementComponent;
class UCurveVector;
class UGameplayTasksComponent;
enum class ERootMotionFinishVelocityMode : uint8;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FApplyTargetRootMotionMoveToForceDelegate);

class AActor;


/** 指定AvatarActor移动到指定位置 */
UCLASS()
class PROJECTGASRPG_API UMageAT_ApplyTargetRootMoveToForce : public UAbilityTask_ApplyRootMotion_Base
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FApplyTargetRootMotionMoveToForceDelegate OnTimedOut;

	UPROPERTY(BlueprintAssignable)
	FApplyTargetRootMotionMoveToForceDelegate OnTimedOutAndDestinationReached;

	/** Apply force to character's movement */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UMageAT_ApplyTargetRootMoveToForce* ApplyTargetRootMotionMoveToForce(UGameplayAbility* OwningAbility, FName TaskInstanceName,AActor* TargetAvatarActor, FVector TargetLocation, float Duration, bool bSetNewMovementMode, EMovementMode MovementMode, bool bRestrictSpeedToExpected, UCurveVector* PathOffsetCurve, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish);

	/** Tick function for this task, if bTickingTask == true */
	virtual void TickTask(float DeltaTime) override;

	virtual void PreDestroyFromReplication() override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

protected:

	virtual void SharedInitAndApply() override;

protected:
	UPROPERTY(Replicated)
	AActor* TargetAvatarActor = nullptr;

	UPROPERTY(Replicated)
	FVector StartLocation;

	UPROPERTY(Replicated)
	FVector TargetLocation;

	UPROPERTY(Replicated)
	float Duration;

	UPROPERTY(Replicated)
	bool bSetNewMovementMode = false;

	UPROPERTY(Replicated)
	TEnumAsByte<EMovementMode> NewMovementMode = EMovementMode::MOVE_Walking;

	/** If enabled, we limit velocity to the initial expected velocity to go distance to the target over Duration.
	 *  This prevents cases of getting really high velocity the last few frames of the root motion if you were being blocked by
	 *  collision. Disabled means we do everything we can to velocity during the move to get to the TargetLocation. */
	UPROPERTY(Replicated)
	bool bRestrictSpeedToExpected = false;

	UPROPERTY(Replicated)
	TObjectPtr<UCurveVector> PathOffsetCurve = nullptr;

	EMovementMode PreviousMovementMode = EMovementMode::MOVE_None;
	uint8 PreviousCustomMovementMode = 0;
};
