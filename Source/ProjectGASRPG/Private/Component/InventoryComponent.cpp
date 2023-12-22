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

void UInventoryComponent::GetNearbyItems()
{
	TArray<AActor*> AllItems;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMageItem::StaticClass(), AllItems);
	
	for (AActor* Item : AllItems)
	{
		if(AMageItem* MageItem = Cast<AMageItem>(Item))
		{
			// 判断物品是否在拾取范围内
			if(FVector::Distance(MageItem->GetActorLocation(), GetOwner()->GetActorLocation()) <= PickUpDistance)
			{
				AddItem(MageItem);
			}
			else
			{
				DeleteItem(MageItem);
			}
		}
	}
}

void UInventoryComponent::AddItem(AMageItem* Item)
{
	NearbyItems.AddUnique(Item); //注意这里使用了AddUnique，防止重复添加
}

void UInventoryComponent::DeleteItem(AMageItem* Item)
{
	if (NearbyItems.Contains(Item))
	{
		NearbyItems.Remove(Item);
	}
}



