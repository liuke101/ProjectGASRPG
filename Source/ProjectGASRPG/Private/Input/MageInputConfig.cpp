// 


#include "Input/MageInputConfig.h"

const UInputAction* UMageInputConfig::FindInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for(auto Action : AbilityInputActions)
	{
		if(Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}
	
	if(bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("没有找到 AbilityInputAction 的 InputTag [%s]，在InputConfig [%s]"), *InputTag.ToString(), *GetNameSafe(this));
	}
	
	return nullptr;
}
