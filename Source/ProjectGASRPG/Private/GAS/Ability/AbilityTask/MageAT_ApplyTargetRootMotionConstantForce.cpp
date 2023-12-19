// 


#include "GAS/Ability/AbilityTask/MageAT_ApplyTargetRootMotionConstantForce.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Net/UnrealNetwork.h"


UMageAT_ApplyTargetRootMotionConstantForce::UMageAT_ApplyTargetRootMotionConstantForce(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	StrengthOverTime = nullptr;
}


UMageAT_ApplyTargetRootMotionConstantForce* UMageAT_ApplyTargetRootMotionConstantForce::
ApplyTargetRootMotionConstantForce(UGameplayAbility* OwningAbility, FName TaskInstanceName, AActor* TargetAvatarActor,
	FVector WorldDirection, float Strength, float Duration, bool bIsAdditive, UCurveFloat* StrengthOverTime,
	ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish,
	bool bEnableGravity)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(Duration);

	UMageAT_ApplyTargetRootMotionConstantForce* MyTask = NewAbilityTask<UMageAT_ApplyTargetRootMotionConstantForce>(OwningAbility, TaskInstanceName);

	MyTask->ForceName = TaskInstanceName;
	MyTask->TargetAvatarActor = TargetAvatarActor;
	MyTask->WorldDirection = WorldDirection.GetSafeNormal();
	MyTask->Strength = Strength;
	MyTask->Duration = Duration;
	MyTask->bIsAdditive = bIsAdditive;
	MyTask->StrengthOverTime = StrengthOverTime;
	MyTask->FinishVelocityMode = VelocityOnFinishMode;
	MyTask->FinishSetVelocity = SetVelocityOnFinish;
	MyTask->FinishClampVelocity = ClampVelocityOnFinish;
	MyTask->bEnableGravity = bEnableGravity;
	MyTask->SharedInitAndApply();

	return MyTask;
}

void UMageAT_ApplyTargetRootMotionConstantForce::SharedInitAndApply()
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetAvatarActor);
	
	if (ASC && ASC->AbilityActorInfo->MovementComponent.IsValid())
	{
		MovementComponent = Cast<UCharacterMovementComponent>(ASC->AbilityActorInfo->MovementComponent.Get());
		StartTime = GetWorld()->GetTimeSeconds();
		EndTime = StartTime + Duration;

		if (MovementComponent)
		{
			ForceName = ForceName.IsNone() ? FName("AbilityTaskApplyRootMotionConstantForce"): ForceName;
			TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
			ConstantForce->InstanceName = ForceName;
			ConstantForce->AccumulateMode = bIsAdditive ? ERootMotionAccumulateMode::Additive : ERootMotionAccumulateMode::Override;
			ConstantForce->Priority = 5;
			ConstantForce->Force = WorldDirection * Strength;
			ConstantForce->Duration = Duration;
			ConstantForce->StrengthOverTime = StrengthOverTime;
			ConstantForce->FinishVelocityParams.Mode = FinishVelocityMode;
			ConstantForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
			ConstantForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
			if (bEnableGravity)
			{
				ConstantForce->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
			}
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(ConstantForce);

			if (Ability)
			{
				Ability->SetMovementSyncPoint(ForceName);
			}
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_ApplyRootMotionConstantForce called in Ability %s with null MovementComponent; Task Instance Name %s."), 
			Ability ? *Ability->GetName() : TEXT("NULL"), 
			*InstanceName.ToString());
	}
}


void UMageAT_ApplyTargetRootMotionConstantForce::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	if (TargetAvatarActor)
	{
		const bool bTimedOut = HasTimedOut();
		const bool bIsInfiniteDuration = Duration < 0.f;

		if (!bIsInfiniteDuration && bTimedOut)
		{
			// Task has finished
			bIsFinished = true;
			if (!bIsSimulating)
			{
				TargetAvatarActor->ForceNetUpdate();
				if (ShouldBroadcastAbilityTaskDelegates())
				{
					OnFinish.Broadcast();
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

void UMageAT_ApplyTargetRootMotionConstantForce::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, WorldDirection);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, TargetAvatarActor);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, Strength);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, Duration);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, bIsAdditive);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, StrengthOverTime);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionConstantForce, bEnableGravity);
}


void UMageAT_ApplyTargetRootMotionConstantForce::PreDestroyFromReplication()
{
	bIsFinished = true;
	EndTask();
}


void UMageAT_ApplyTargetRootMotionConstantForce::OnDestroy(bool AbilityIsEnding)
{
	if (MovementComponent)
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
	}

	Super::OnDestroy(AbilityIsEnding);
}
