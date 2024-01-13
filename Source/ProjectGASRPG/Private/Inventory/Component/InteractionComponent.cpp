#include "Inventory/Component/InteractionComponent.h"

#include "GAS/MageAbilitySystemLibrary.h"
#include "Inventory/Interface/InteractionInterface.h"
#include "Inventory/Item/MageItem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/WidgetController/OverlayWidgetController.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		PerformInteractionCheck();
	}
}


void UInteractionComponent::PerformInteractionCheck()
{
	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();
	
	if(const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		const FVector TraceStart = OwnerPawn->GetPawnViewLocation();
		const FVector TraceEnd = TraceStart + OwnerPawn->GetControlRotation().Vector() * InteractionCheckDistance;

		//只允许朝角色面向进行射线检测
		const float LookDirection = FVector::DotProduct(OwnerPawn->GetActorForwardVector(),OwnerPawn->GetViewRotation().Vector());
		if(LookDirection <= 0.f) return;
		
		//射线检测
		FHitResult HitResult;
		//TraceTypeQuery1默认为Visibility
		UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd, TraceTypeQuery1, false, {GetOwner()}, EDrawDebugTrace::ForDuration, HitResult, true, FLinearColor::Red, FLinearColor::Green, 2.f);
		
		if(HitResult.bBlockingHit)
		{
			if(HitResult.GetActor()->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
			{
				if(HitResult.GetActor() != InteractionData.CurrentInteractableActor)
				{
					FoundInteractableActor(HitResult.GetActor());
					return;
				}
				else
				{
					return;
				}
			}
		}

		NotFoundInteractableActor();
	}
}

void UInteractionComponent::FoundInteractableActor(AActor* NewInteractableActor)
{
	if(IsInteracting())
	{
		EndInteract();
	}

	if(InteractionData.CurrentInteractableActor)
	{
		TargetInteractableObject = InteractionData.CurrentInteractableActor;
		TargetInteractableObject->EndFocus();
	}

	InteractionData.CurrentInteractableActor = NewInteractableActor;
	TargetInteractableObject = NewInteractableActor;

	//广播InteractionData, 更新InteractionWidget显示，在WBP_InteractionWidget中绑定
	UMageAbilitySystemLibrary::GetOverlayWidgetController(GetWorld())->OnInteractableDataChanged.Broadcast(TargetInteractableObject->GetInteractableData());

	TargetInteractableObject->BeginFocus();
}

void UInteractionComponent::NotFoundInteractableActor()
{
	if(IsInteracting())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Interaction);
	}

	if(InteractionData.CurrentInteractableActor)
	{
		if(IsValid(TargetInteractableObject.GetObject()))
		{
			TargetInteractableObject->EndFocus();
		}
	}

	//隐藏widget, 在WBP_InteractionWidget中绑定
	UMageAbilitySystemLibrary::GetOverlayWidgetController(GetWorld())->HideInteractionWidget.Broadcast();

	InteractionData.CurrentInteractableActor = nullptr;
	TargetInteractableObject = nullptr;
}

void UInteractionComponent::BeginInteract()
{
	//确定交互对象不是同一个
	PerformInteractionCheck();

	if(InteractionData.CurrentInteractableActor)
	{
		//显示拾取信息Widget
		if(AMageItem* Item = Cast<AMageItem>(InteractionData.CurrentInteractableActor))
		{
			UMageAbilitySystemLibrary::GetOverlayWidgetController(GetWorld())->SetMageItem(Item);
		}
		
		if(IsValid(TargetInteractableObject.GetObject()))
		{
			TargetInteractableObject->BeginInteract();
			if(FMath::IsNearlyZero(TargetInteractableObject->GetInteractableData().InteractionDuration,0.1f))
			{
				Interact();
			}
			else
			{
				GetWorld()->GetTimerManager().SetTimer(TimerHandle_Interaction,this,&UInteractionComponent::Interact,TargetInteractableObject->GetInteractableData().InteractionDuration,false);
			}
		}
	}
}

void UInteractionComponent::Interact()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Interaction);

	if(IsValid(TargetInteractableObject.GetObject()))
	{
		TargetInteractableObject->Interact();
	}
}

void UInteractionComponent::EndInteract()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Interaction);
	
	if(IsValid(TargetInteractableObject.GetObject()))
	{
		TargetInteractableObject->EndInteract();
	}
}
