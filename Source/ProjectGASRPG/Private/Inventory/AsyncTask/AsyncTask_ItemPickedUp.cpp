#include "Inventory/AsyncTask/AsyncTask_ItemPickedUp.h"
#include "UI/WidgetController/MageWidgetController.h"

UAsyncTask_ItemPickedUp* UAsyncTask_ItemPickedUp::ListenForItemPickedUp(UMageWidgetController* MageWidgetController)
{
	UAsyncTask_ItemPickedUp* WaitForItemPickedUp = NewObject<UAsyncTask_ItemPickedUp>();
	WaitForItemPickedUp->MageWidgetController = MageWidgetController;

	if(!IsValid(MageWidgetController))
	{
		WaitForItemPickedUp->EndTask();
		return nullptr;
	}

	// 绑定拾取物品的物品信息委托
	MageWidgetController->OnItemPickedUp.AddUObject(WaitForItemPickedUp, &UAsyncTask_ItemPickedUp::ItemPickedUpCallback);

	return WaitForItemPickedUp;
}

void UAsyncTask_ItemPickedUp::EndTask()
{
	if(IsValid(MageWidgetController))
	{
		MageWidgetController->OnItemPickedUp.RemoveAll(this);
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTask_ItemPickedUp::ItemPickedUpCallback(const AMageItem* MageItem) const
{
	OnItemPickedUp.Broadcast(MageItem);
}

