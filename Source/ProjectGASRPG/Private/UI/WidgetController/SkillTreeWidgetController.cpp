// 


#include "UI/WidgetController/SkillTreeWidgetController.h"

#include "GAS/MageAbilitySystemComponent.h"

void USkillTreeWidgetController::BroadcastInitialValue()
{
	BroadcastAbilityInfo();
}

void USkillTreeWidgetController::BindCallbacks()
{
	if(GetMageASC()->bCharacterAbilitiesGiven)
	{
		BroadcastAbilityInfo();
	}
	else 
	{
		GetMageASC()->AbilitiesGiven.AddUObject(this, &USkillTreeWidgetController::BroadcastAbilityInfo);
	}
}
