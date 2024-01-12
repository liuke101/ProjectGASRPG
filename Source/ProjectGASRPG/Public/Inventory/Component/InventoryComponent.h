#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class AMageItem;

//增删改查委托
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryItemCRUD, const AMageItem* Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	FInventoryItemCRUD OnItemAdded;
	FInventoryItemCRUD OnItemRemoved;
protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	//增
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(AMageItem* Item);

	//删
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(AMageItem* Item);

	//改
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapItem(AMageItem* ItemA, AMageItem* ItemB);

	//查
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AMageItem* FindItemByTag(const FGameplayTag& Tag);

	/** 存储的Item */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<AMageItem*> Items;
};
