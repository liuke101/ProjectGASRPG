#include "Component/InventoryComponent.h"

#include "Item/MageItem.h"
#include "Kismet/GameplayStatics.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInventoryComponent::AddItem(AMageItem* Item)
{
	Items.AddUnique(Item);
	OnItemAdded.Broadcast(Item);
}

void UInventoryComponent::RemoveItem(AMageItem* Item)
{
	if(!Items.Contains(Item))
	{
		return;
	}
	
	Items.Remove(Item);
	OnItemRemoved.Broadcast(Item);
}

void UInventoryComponent::SwapItem(AMageItem* ItemA, AMageItem* ItemB)
{
	//交换ID
	Swap(ItemA, ItemB);
}

AMageItem* UInventoryComponent::FindItemByTag(const FGameplayTag& Tag)
{
	for (const auto Item : Items)
	{
		if(Item->GetItemTag() == Tag)
		{
			return Item;
		}
	}
	return nullptr;
}



