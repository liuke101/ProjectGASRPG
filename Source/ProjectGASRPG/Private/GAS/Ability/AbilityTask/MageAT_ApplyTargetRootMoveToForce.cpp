// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/Ability/AbilityTask/MageAT_ApplyTargetRootMoveToForce.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MageAT_ApplyTargetRootMoveToForce)

UMageAT_ApplyTargetRootMoveToForce* UMageAT_ApplyTargetRootMoveToForce::ApplyTargetRootMotionMoveToForce(UGameplayAbility* OwningAbility, FName TaskInstanceName,AActor* TargetAvatarActor,FVector TargetLocation, float Duration, bool bSetNewMovementMode, EMovementMode MovementMode, bool bRestrictSpeedToExpected, UCurveVector* PathOffsetCurve, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(Duration);

	UMageAT_ApplyTargetRootMoveToForce* MyTask = NewAbilityTask<UMageAT_ApplyTargetRootMoveToForce>(OwningAbility, TaskInstanceName);

	MyTask->ForceName = TaskInstanceName;
	MyTask->TargetAvatarActor = TargetAvatarActor;
	MyTask->TargetLocation = TargetLocation;
	MyTask->Duration = FMath::Max(Duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
	MyTask->bSetNewMovementMode = bSetNewMovementMode;
	MyTask->NewMovementMode = MovementMode;
	MyTask->bRestrictSpeedToExpected = bRestrictSpeedToExpected;
	MyTask->PathOffsetCurve = PathOffsetCurve;
	MyTask->FinishVelocityMode = VelocityOnFinishMode;
	MyTask->FinishSetVelocity = SetVelocityOnFinish;
	MyTask->FinishClampVelocity = ClampVelocityOnFinish;
	if (TargetAvatarActor)
	{
		MyTask->StartLocation = TargetAvatarActor->GetActorLocation();
	}
	else
	{
		checkf(false, TEXT("UMageAT_ApplyTargetRootMoveToForce called without valid Target avatar actor to get start location from."));
		MyTask->StartLocation = TargetLocation;
	}
	MyTask->SharedInitAndApply();

	return MyTask;
}

void UMageAT_ApplyTargetRootMoveToForce::SharedInitAndApply()
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetAvatarActor);
	
	if (ASC && ASC->AbilityActorInfo->MovementComponent.IsValid())
	{
		MovementComponent = Cast<UCharacterMovementComponent>(ASC->AbilityActorInfo->MovementComponent.Get());
		StartTime = GetWorld()->GetTimeSeconds();
		EndTime = StartTime + Duration;

		if (MovementComponent)
		{
			if (bSetNewMovementMode)
			{
				PreviousMovementMode = MovementComponent->MovementMode;
				PreviousCustomMovementMode = MovementComponent->CustomMovementMode;
				MovementComponent->SetMovementMode(NewMovementMode);
			}

			ForceName = ForceName.IsNone() ? FName("AbilityTaskApplyRootMotionMoveToForce") : ForceName;
			TSharedPtr<FRootMotionSource_MoveToForce> MoveToForce = MakeShared<FRootMotionSource_MoveToForce>();
			MoveToForce->InstanceName = ForceName;
			MoveToForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			MoveToForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::UseSensitiveLiftoffCheck);
			MoveToForce->Priority = 1000;
			MoveToForce->TargetLocation = TargetLocation;
			MoveToForce->StartLocation = StartLocation;
			MoveToForce->Duration = Duration;
			MoveToForce->bRestrictSpeedToExpected = bRestrictSpeedToExpected;
			MoveToForce->PathOffsetCurve = PathOffsetCurve;
			MoveToForce->FinishVelocityParams.Mode = FinishVelocityMode;
			MoveToForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
			MoveToForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(MoveToForce);

			if (Ability)
			{
				Ability->SetMovementSyncPoint(ForceName);
			}
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UMageAT_ApplyTargetRootMoveToForce called in Ability %s with null MovementComponent; Task Instance Name %s."), 
			Ability ? *Ability->GetName() : TEXT("NULL"), 
			*InstanceName.ToString());
	}
}


void UMageAT_ApplyTargetRootMoveToForce::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	if (TargetAvatarActor)
	{
		const bool bTimedOut = HasTimedOut();
		const float ReachedDestinationDistanceSqr = 50.f * 50.f;
		const bool bReachedDestination = FVector::DistSquared(TargetLocation, TargetAvatarActor->GetActorLocation()) < ReachedDestinationDistanceSqr;

		if (bTimedOut)
		{
			// Task has finished
			bIsFinished = true;
			if (!bIsSimulating)
			{
				TargetAvatarActor->ForceNetUpdate();
				if (bReachedDestination)
				{
					if (ShouldBroadcastAbilityTaskDelegates())
					{
						OnTimedOutAndDestinationReached.Broadcast();
					}
				}
				else
				{
					if (ShouldBroadcastAbilityTaskDelegates())
					{
						OnTimedOut.Broadcast();
					}
				}
				EndTask();
			}
		}
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UMageAT_ApplyTargetRootMoveToForce::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, TargetAvatarActor);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, StartLocation);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, TargetLocation);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, Duration);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, bSetNewMovementMode);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, NewMovementMode);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, bRestrictSpeedToExpected);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMoveToForce, PathOffsetCurve);
}

void UMageAT_ApplyTargetRootMoveToForce::PreDestroyFromReplication()
{
	bIsFinished = true;
	EndTask();
}

void UMageAT_ApplyTargetRootMoveToForce::OnDestroy(bool AbilityIsEnding)
{
	if (MovementComponent)
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);

		if (bSetNewMovementMode)
		{
 			MovementComponent->SetMovementMode(PreviousMovementMode, PreviousCustomMovementMode);
		}
	}

	Super::OnDestroy(AbilityIsEnding);
}

