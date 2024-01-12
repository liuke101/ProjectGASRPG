#include "Inventory/Data/ItemDataAsset.h"

FMageItemInfo UItemDataAsset::FindMageItemInfoForTag(const FGameplayTag& ItemTag, bool blogNotFound) const
{
	for(const FMageItemInfo& MageItemInfo : MageItemInfos)
	{
		if(MageItemInfo.ItemTag.MatchesTagExact(ItemTag)) //等价if(AttributeInfo.AttributeTag == AttributeTag)
		{
			return MageItemInfo;
		}
	}

	if(blogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Tag [%s] 没有在MageItemInfos [%s] 中找到"), *ItemTag.ToString(), *GetNameSafe(this));
	}
	
	return FMageItemInfo();
}
