// 

#pragma once

#include "CoreMinimal.h"
#include "MageUserWidget.h"
#include "Inventory/Item/MageItem.h"
#include "InventorySlot.generated.h"

class AMageItem;
/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UInventorySlot : public UMageUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventorySlot")
	bool bIsEmpty = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventorySlot")
	TObjectPtr<AMageItem> Item;
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "InventorySlot")
	void SetItem(AMageItem* NewItem);
	
	UFUNCTION(BlueprintCallable, Category = "InventorySlot")
	void SwapItem(UInventorySlot* OldSlot, UInventorySlot* NewSlot);

};
