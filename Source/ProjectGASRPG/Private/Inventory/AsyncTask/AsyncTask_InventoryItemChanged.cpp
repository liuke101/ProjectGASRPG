#include "Inventory/AsyncTask/AsyncTask_InventoryItemChanged.h"
#include "Inventory/Component/InventoryComponent.h"

UAsyncTask_InventoryItemChanged* UAsyncTask_InventoryItemChanged::ListenForInventoryItemChanged(
	UInventoryComponent* InventoryComponent)
{
	UAsyncTask_InventoryItemChanged* WaitForInventoryItemChanged = NewObject<UAsyncTask_InventoryItemChanged>();

	if(!IsValid(InventoryComponent))
	{
		WaitForInventoryItemChanged->EndTask();
		return nullptr;
	}

	// 绑定物品增删改查委托
	InventoryComponent->OnItemAdded.AddUObject(WaitForInventoryItemChanged, &UAsyncTask_InventoryItemChanged::ItemAddedCallback);
	
	InventoryComponent->OnItemRemoved.AddUObject(WaitForInventoryItemChanged, &UAsyncTask_InventoryItemChanged::ItemRemovedCallback);

	InventoryComponent->OnItemUpdate.AddUObject(WaitForInventoryItemChanged, &UAsyncTask_InventoryItemChanged::ItemUpdateCallback);

	return WaitForInventoryItemChanged;
}

void UAsyncTask_InventoryItemChanged::EndTask()
{
	if(IsValid(InventoryComponent))
	{
		InventoryComponent->OnItemAdded.RemoveAll(this);
		InventoryComponent->OnItemRemoved.RemoveAll(this);
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTask_InventoryItemChanged::ItemAddedCallback(const AMageItem* Item) const
{
	OnItemAdded.Broadcast(Item);
}

void UAsyncTask_InventoryItemChanged::ItemRemovedCallback(const AMageItem* Item) const
{
	OnItemRemoved.Broadcast(Item);
}

void UAsyncTask_InventoryItemChanged::ItemUpdateCallback(const AMageItem* Item) const
{
	OnItemUpdate.Broadcast(Item);
}

