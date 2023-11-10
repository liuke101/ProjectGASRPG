// 


#include "GAS/Data/AbilityDataAsset.h"

FMageAbilityInfo UAbilityDataAsset::FindAbilityInfoForTag(const FGameplayTag& AbilityTag, bool blogNotFound) const
{
	for(const FMageAbilityInfo& AbilityInfo : AbilityInfos)
	{
		if(AbilityInfo.AbilityTag.MatchesTagExact(AbilityTag)) 
		{
			return AbilityInfo;
		}
	}

	if(blogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Tag [%s] 没有在AbilityInfos [%s] 中找到"), *AbilityTag.ToString(), *GetNameSafe(this));
	}
	
	return FMageAbilityInfo();
}
