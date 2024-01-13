// 

#pragma once

#include "CoreMinimal.h"
#include "MageItem.h"
#include "PickupItem.generated.h"

UCLASS()
class PROJECTGASRPG_API APickupItem : public AMageItem
{
	GENERATED_BODY()

public:
	APickupItem();

protected:
	virtual void BeginPlay() override;

public:
	void InitDrop(AMageItem* ItemToDrop, const int32 InQuantity);
	virtual void UpdateInteractableData() override;

	virtual void BeginInteract() override; //按下F触发
	virtual void Interact() override; //按下F后持续触发
	virtual void EndInteract() override; //松开F触发
	void PickUp();
};
