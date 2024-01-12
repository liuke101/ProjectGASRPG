// 

#pragma once

#include "CoreMinimal.h"
#include "MageUserWidget.h"
#include "InventoryPanel.generated.h"

class UInventorySlot;
/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UInventoryPanel : public UMageUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventorySlot* GetFirstEmptySlot(const TArray<UInventorySlot*>& InventorySlots) const;
};
