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
	Items.AddUnique(Item); //注意这里使用了AddUnique，防止重复添加
}

void UInventoryComponent::DeleteItem(AMageItem* Item)
{
	if (Items.Contains(Item))
	{
		Items.Remove(Item);
	}
}

AMageItem* UInventoryComponent::FindItemByTag(const FGameplayTag& Tag)
{
	for(const auto Item : Items)
	{
		if(Item->GetItemTag().MatchesTagExact(Tag))
		{
			return Item;
		}
	}
	return nullptr;
}



