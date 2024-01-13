#include "Inventory/Component/InventoryComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Inventory/Item/MageItem.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

FItemAddResult UInventoryComponent::HandleAddItem(AMageItem* InItem)
{
	
	const int32 InitialRequestedAddAmount = InItem->Quantity;

	//如果物品不可堆叠
	if(!InItem->ItemNumericData.bIsStackable)
	{
		return HandleNonStackableItems(InItem, InitialRequestedAddAmount);
	}
	else //如果物品可堆叠
	{
		const int32 StackableAmountAdded = HandleStackableItems(InItem, InitialRequestedAddAmount);
		
		if(StackableAmountAdded == InitialRequestedAddAmount)
		{
			return FItemAddResult::AddedAll(FText::Format(FText::FromString("Inventory add all {0} x{1}"), InItem->ItemTextData.Name, InitialRequestedAddAmount),InitialRequestedAddAmount);
		}

		if(StackableAmountAdded < InitialRequestedAddAmount && StackableAmountAdded > 0)
		{
			return FItemAddResult::AddedPartial(FText::Format(FText::FromString("Inventory add Partial {0} x{1}"), InItem->ItemTextData.Name, StackableAmountAdded),StackableAmountAdded);
		}

		if(StackableAmountAdded<=0)
		{
			return FItemAddResult::AddedNone(FText::Format(FText::FromString("Inventory is full, cannot add {0}"), InItem->ItemTextData.Name));
		}
	}

	return FItemAddResult::AddedNone(FText::FromString("add failed"));
}

void UInventoryComponent::AddNewItem(AMageItem* InItem, const int32 AddAmount)
{
	AMageItem* NewItem;

	//如果Item不在背包中（即在世界中），那么直接使用该Item
	if(!InItem->bIsInInventory)
	{
		NewItem = InItem;
		InItem->bIsInInventory = true; //拾取后在背包中
	}
	else //如果Item在背包中，那么创建Item的副本。这对于拆分可堆叠Item很有用
	{
		NewItem = InItem->CreateItemCopy();
	}
	
	NewItem->SetQuantity(AddAmount);
	InventoryContents.Add(NewItem);
	OnItemAdded.Broadcast(NewItem);
}


void UInventoryComponent::RemoveSingleInstanceOfItem(AMageItem* ItemToRemove)
{
	if(!InventoryContents.Contains(ItemToRemove))
	{
		return;
	}
	
	InventoryContents.Remove(ItemToRemove);
	OnItemRemoved.Broadcast(ItemToRemove);
}

int32 UInventoryComponent::RemoveAmountOfItem(AMageItem* InItem, int32 RemoveAmount)
{
	const int32 ActualAmountToRemove = FMath::Min(RemoveAmount, InItem->Quantity);
	InItem->SetQuantity(InItem->Quantity - ActualAmountToRemove);

	//如果Item数量归0，那么移除该Item
	if(ActualAmountToRemove == InItem->Quantity)
	{
		RemoveSingleInstanceOfItem(InItem);
	}

	OnItemRemoved.Broadcast(InItem);
	return ActualAmountToRemove;
}


void UInventoryComponent::SplitItemStack(AMageItem* InItem, int32 SplitAmount)
{
	if(InventoryContents.Num() + 1 <= InventorySlotsCapacity)
	{
		RemoveAmountOfItem(InItem, SplitAmount);
		AddNewItem(InItem, SplitAmount);
	}
}

AMageItem* UInventoryComponent::FindMatchingItem(AMageItem* InItem)
{
	if(InItem)
	{
		if(InventoryContents.Contains(InItem))
		{
			return InItem;
		}
	}
	return nullptr;
}

AMageItem* UInventoryComponent::FindItemByTag(const FGameplayTag& Tag)
{
	for (const auto Item : InventoryContents)
	{
		if(Item->ItemTag == Tag)
		{
			return Item;
		}
	}
	return nullptr;
}

AMageItem* UInventoryComponent::FindNextItemByTag(AMageItem* InItem)
{
	if(InItem)
	{
		//FindByPredicate使用谓词查找，这里使用lambda表达式
		if(const auto Result = InventoryContents.FindByPredicate([&InItem](const AMageItem* Item)
		{
			return Item->ItemTag == InItem->ItemTag;
		}))
		{
			return *Result;
		}
	}
	
	return nullptr;
}


AMageItem* UInventoryComponent::FindNextPartialStack(AMageItem* InItem)
{
	if(const auto Result = InventoryContents.FindByPredicate([&InItem](const AMageItem* Item)
	{
		return Item->ItemTag == InItem->ItemTag && !Item->IsFullItemStack();
	}))
	{
		return *Result;
	}
	
	return nullptr;
}

FItemAddResult UInventoryComponent::HandleNonStackableItems(AMageItem* InItem, int32 RequestedAddAmount)
{
	//如果背包满了
	if(InventoryContents.Num() + 1 > InventorySlotsCapacity)
	{
		return FItemAddResult::AddedNone(FText::Format(FText::FromString("Inventory is full, cannot add {0}"), InItem->ItemTextData.Name));
	}

	AddNewItem(InItem, RequestedAddAmount);
	
	return FItemAddResult::AddedAll(FText::Format(FText::FromString("Inventory add all {0} x{1}"), InItem->ItemTextData.Name, RequestedAddAmount),RequestedAddAmount);
}

int32 UInventoryComponent::HandleStackableItems(AMageItem* InItem, int32 RequestedAddAmount)
{
	return 0;
}

int32 UInventoryComponent::CalcNumberForFullStack(AMageItem* StackableItem, int32 InitialRequestedAddAmount)
{
	const int32 AddAmountToMakeFullStack = StackableItem->ItemNumericData.MaxStackSize - StackableItem->Quantity;
	return FMath::Min(AddAmountToMakeFullStack, InitialRequestedAddAmount);
}




