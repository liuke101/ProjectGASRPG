#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class AMageItem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	/** 获取附近的物品 */
	void GetNearbyItems();

	void AddItem(AMageItem* Item);
	void DeleteItem(AMageItem* Item);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageItem|PickUp")
	float PickUpDistance = 200.f;

private:
	/** 附近的物品 */
	TArray<AMageItem*> NearbyItems;
};
