// 

#pragma once

#include "CoreMinimal.h"
#include "MageUserWidget.h"
#include "InventorySlot.generated.h"

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
};
