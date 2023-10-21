// 


#include "GAS/Data/AttributeDataAsset.h"

FMageAttributeInfo UAttributeDataAsset::FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool blogNotFound) const
{
	for(const FMageAttributeInfo& AttributeInfo : AttributeInfos)
	{
		if(AttributeInfo.AttributeTag.MatchesTagExact(AttributeTag)) //等价if(AttributeInfo.AttributeTag == AttributeTag)
		{
			return AttributeInfo;
		}
	}

	if(blogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Tag [%s] 没有在AttributeInfos [%s] 中找到"), *AttributeTag.ToString(), *GetNameSafe(this));
	}
	
	return FMageAttributeInfo();
}
