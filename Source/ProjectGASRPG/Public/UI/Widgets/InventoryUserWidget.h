// 

#pragma once

#include "CoreMinimal.h"
#include "MageUserWidget.h"
#include "InventoryUserWidget.generated.h"

class UInventorySlotUserWidget;
/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UInventoryUserWidget : public UMageUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventorySlotUserWidget* GetFirstEmptySlot(const TArray<UInventorySlotUserWidget*>& InventorySlots) const;
};
