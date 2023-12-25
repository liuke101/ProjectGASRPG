// 


#include "UI/Widgets/InventoryUserWidget.h"

#include "UI/Widgets/InventorySlotUserWidget.h"

UInventorySlotUserWidget* UInventoryUserWidget::GetFirstEmptySlot(const TArray<UInventorySlotUserWidget*>& InventorySlots) const
{
	for (const auto InventorySlot : InventorySlots)
	{
		if (InventorySlot->bIsEmpty)
		{
			return InventorySlot;
		}
	}

	return nullptr;
}
