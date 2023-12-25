// 

#pragma once

#include "CoreMinimal.h"
#include "MageUserWidget.h"
#include "InventorySlotUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UInventorySlotUserWidget : public UMageUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventorySlot")
	bool bIsEmpty = true;
};
