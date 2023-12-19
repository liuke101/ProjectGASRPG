#include "GAS/Ability/AbilityTask/MageAT_ApplyTargetRootMotionRadialForce.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Net/UnrealNetwork.h"

UMageAT_ApplyTargetRootMotionRadialForce::UMageAT_ApplyTargetRootMotionRadialForce(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	StrengthDistanceFalloff = nullptr;
	StrengthOverTime = nullptr;
	bUseFixedWorldDirection = false;
}

UMageAT_ApplyTargetRootMotionRadialForce* UMageAT_ApplyTargetRootMotionRadialForce::ApplyTargetRootMotionRadialForce(
	UGameplayAbility* OwningAbility, FName TaskInstanceName, AActor* TargetAvatarActor, FVector Location,
	AActor* LocationActor, float Strength, float Duration, float Radius, bool bIsPush, bool bIsAdditive, bool bNoZForce,
	UCurveFloat* StrengthDistanceFalloff, UCurveFloat* StrengthOverTime, bool bUseFixedWorldDirection,
	FRotator FixedWorldDirection, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish,
	float ClampVelocityOnFinish)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(Duration);

	UMageAT_ApplyTargetRootMotionRadialForce* MyTask = NewAbilityTask<UMageAT_ApplyTargetRootMotionRadialForce>(OwningAbility, TaskInstanceName);

	MyTask->ForceName = TaskInstanceName;
	MyTask->TargetAvatarActor = TargetAvatarActor;
	MyTask->Location = Location;
	MyTask->LocationActor = LocationActor;
	MyTask->Strength = Strength;
	MyTask->Radius = FMath::Max(Radius, SMALL_NUMBER); // No zero radius
	MyTask->Duration = Duration;
	MyTask->bIsPush = bIsPush;
	MyTask->bIsAdditive = bIsAdditive;
	MyTask->bNoZForce = bNoZForce;
	MyTask->StrengthDistanceFalloff = StrengthDistanceFalloff;
	MyTask->StrengthOverTime = StrengthOverTime;
	MyTask->bUseFixedWorldDirection = bUseFixedWorldDirection;
	MyTask->FixedWorldDirection = FixedWorldDirection;
	MyTask->FinishVelocityMode = VelocityOnFinishMode;
	MyTask->FinishSetVelocity = SetVelocityOnFinish;
	MyTask->FinishClampVelocity = ClampVelocityOnFinish;
	MyTask->SharedInitAndApply();

	return MyTask;
}
void UMageAT_ApplyTargetRootMotionRadialForce::SharedInitAndApply()
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetAvatarActor);
	
	if (ASC && ASC->AbilityActorInfo->MovementComponent.IsValid())
	{
		MovementComponent = Cast<UCharacterMovementComponent>(ASC->AbilityActorInfo->MovementComponent.Get());
		StartTime = GetWorld()->GetTimeSeconds();
		EndTime = StartTime + Duration;

		if (MovementComponent)
		{
			ForceName = ForceName.IsNone() ? FName("AbilityTaskApplyRootMotionRadialForce") : ForceName;
			TSharedPtr<FRootMotionSource_RadialForce> RadialForce = MakeShared<FRootMotionSource_RadialForce>();
			RadialForce->InstanceName = ForceName;
			RadialForce->AccumulateMode = bIsAdditive ? ERootMotionAccumulateMode::Additive : ERootMotionAccumulateMode::Override;
			RadialForce->Priority = 5;
			RadialForce->Location = Location;
			RadialForce->LocationActor = LocationActor;
			RadialForce->Duration = Duration;
			RadialForce->Radius = Radius;
			RadialForce->Strength = Strength;
			RadialForce->bIsPush = bIsPush;
			RadialForce->bNoZForce = bNoZForce;
			RadialForce->StrengthDistanceFalloff = StrengthDistanceFalloff;
			RadialForce->StrengthOverTime = StrengthOverTime;
			RadialForce->bUseFixedWorldDirection = bUseFixedWorldDirection;
			RadialForce->FixedWorldDirection = FixedWorldDirection;
			RadialForce->FinishVelocityParams.Mode = FinishVelocityMode;
			RadialForce->FinishVelocityParams.SetVelocity = FinishSetVelocity;
			RadialForce->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
			RootMotionSourceID = MovementComponent->ApplyRootMotionSource(RadialForce);

			if (Ability)
			{
				Ability->SetMovementSyncPoint(ForceName);
			}
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UAbilityTask_ApplyRootMotionRadialForce called in Ability %s with null MovementComponent; Task Instance Name %s."), 
			Ability ? *Ability->GetName() : TEXT("NULL"), 
			*InstanceName.ToString());
	}
}

void UMageAT_ApplyTargetRootMotionRadialForce::TickTask(float DeltaTime)
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

void UMageAT_ApplyTargetRootMotionRadialForce::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, TargetAvatarActor);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, Location);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, LocationActor);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, Radius);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, Strength);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, Duration);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, bIsPush);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, bIsAdditive);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, bNoZForce);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, StrengthDistanceFalloff);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, StrengthOverTime);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, bUseFixedWorldDirection);
	DOREPLIFETIME(UMageAT_ApplyTargetRootMotionRadialForce, FixedWorldDirection);
}



void UMageAT_ApplyTargetRootMotionRadialForce::PreDestroyFromReplication()
{
	bIsFinished = true;
	EndTask();
}

void UMageAT_ApplyTargetRootMotionRadialForce::OnDestroy(bool AbilityIsEnding)
{
	if (MovementComponent)
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
	}

	Super::OnDestroy(AbilityIsEnding);
}


