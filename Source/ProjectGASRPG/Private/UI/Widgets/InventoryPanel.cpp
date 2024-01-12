// 


#include "UI/Widgets/InventoryPanel.h"

#include "UI/Widgets/InventorySlot.h"

UInventorySlot* UInventoryPanel::GetFirstEmptySlot(const TArray<UInventorySlot*>& InventorySlots) const
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
