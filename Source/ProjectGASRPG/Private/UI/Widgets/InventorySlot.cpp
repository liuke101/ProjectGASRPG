#include "UI/Widgets/InventorySlot.h"

void UInventorySlot::SwapItem(UInventorySlot* OldSlot, UInventorySlot* NewSlot)
{
	if(OldSlot && NewSlot)
	{
		AMageItem* TempItem = OldSlot->Item;
		OldSlot->SetItem(NewSlot->Item);
		NewSlot->SetItem(TempItem);
	}
}
