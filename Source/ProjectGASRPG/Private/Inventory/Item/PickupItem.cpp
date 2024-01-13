#include "Inventory/Item/PickupItem.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Inventory/Component/InteractionComponent.h"
#include "Inventory/Component/InventoryComponent.h"
#include "UI/WidgetController/OverlayWidgetController.h"


APickupItem::APickupItem()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APickupItem::BeginPlay()
{
	Super::BeginPlay();
	
}


void APickupItem::InitDrop(AMageItem* ItemToDrop,const int32 InQuantity)
{
	InQuantity <= 0 ? Quantity = 1 : Quantity = InQuantity;
	UpdateInteractableData();
}

void APickupItem::UpdateInteractableData()
{
	Super::UpdateInteractableData();
}

void APickupItem::PickUp()
{
	if(!IsPendingKillPending())
	{
		if(UInventoryComponent* InventoryComponent = UMageAbilitySystemLibrary::GetInventoryComponent(GetWorld()))
		{
			const FItemAddResult AddResult = InventoryComponent->HandleAddItem(this);
			switch (AddResult.Result)
			{
			case EItemAddResult::IAR_NotItemAdded:
				break;
			case EItemAddResult::IAR_PartialItemAdded:
				UpdateInteractableData();
				
				//广播InteractionData, 更新InteractionWidget显示，在WBP_InteractionWidget中绑定
				UMageAbilitySystemLibrary::GetOverlayWidgetController(GetWorld())->OnInteractableDataChanged.Broadcast(UMageAbilitySystemLibrary::GetInteractionComponent(GetWorld())->TargetInteractableObject->GetInteractableData());
				
				break;
			case EItemAddResult::IAR_AllItemAdded:
				Destroy(); //全部添加进背包后销毁
				break;
			default: ;
			}
			
			UE_LOG(LogTemp, Warning, TEXT("%s"), *AddResult.ResultMessage.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Player InventoryComponent is nullptr"));
		}

		
	}
}

void APickupItem::BeginInteract()
{
}

void APickupItem::Interact()
{
	PickUp();
}

void APickupItem::EndInteract()
{
}


