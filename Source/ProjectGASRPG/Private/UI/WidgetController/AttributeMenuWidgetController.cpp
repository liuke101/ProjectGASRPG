// 


#include "UI/WidgetController/AttributeMenuWidgetController.h"

#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Data/AttributeInfo.h"

void UAttributeMenuWidgetController::BroadcastInitialValue()
{
	const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet);
	checkf(AttributeInfo, TEXT("AttributeInfo为空, 请在BP_AttributeMenuWidgetController中设置"));

	FMageAttributeInfo Info_Health = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Vital_Health, true);
	Info_Health.AttributeValue = MageAttributeSet->GetHealth();
	AttributeInfoDelegate.Broadcast(Info_Health);

	FMageAttributeInfo Info_Mana = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Vital_Mana, true);
	Info_Mana.AttributeValue = MageAttributeSet->GetMana();
	AttributeInfoDelegate.Broadcast(Info_Mana);

	FMageAttributeInfo Info_Strength = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Primary_Strength, true);
	Info_Strength.AttributeValue = MageAttributeSet->GetStrength();
	AttributeInfoDelegate.Broadcast(Info_Strength);

	FMageAttributeInfo Info_Intelligence = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Primary_Intelligence, true);
	Info_Intelligence.AttributeValue = MageAttributeSet->GetIntelligence();
	AttributeInfoDelegate.Broadcast(Info_Intelligence);

	FMageAttributeInfo Info_Stamina = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Primary_Stamina, true);
	Info_Stamina.AttributeValue = MageAttributeSet->GetStamina();
	AttributeInfoDelegate.Broadcast(Info_Stamina);

	FMageAttributeInfo Info_Vigor = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Primary_Vigor, true);
	Info_Vigor.AttributeValue = MageAttributeSet->GetVigor();
	AttributeInfoDelegate.Broadcast(Info_Vigor);

	FMageAttributeInfo Info_MaxHealth = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_MaxHealth, true);
	Info_MaxHealth.AttributeValue = MageAttributeSet->GetMaxHealth();
	AttributeInfoDelegate.Broadcast(Info_MaxHealth);
	
	FMageAttributeInfo Info_MaxMana = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_MaxMana, true);
	Info_MaxMana.AttributeValue = MageAttributeSet->GetMaxMana();
	AttributeInfoDelegate.Broadcast(Info_MaxMana);

	FMageAttributeInfo Info_MaxPhysicalAttack = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_MaxPhysicalAttack, true);
	Info_MaxPhysicalAttack.AttributeValue = MageAttributeSet->GetMaxPhysicalAttack();
	AttributeInfoDelegate.Broadcast(Info_MaxPhysicalAttack);
	
	FMageAttributeInfo Info_MinPhysicalAttack = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_MinPhysicalAttack, true);
	Info_MinPhysicalAttack.AttributeValue = MageAttributeSet->GetMinPhysicalAttack();
	AttributeInfoDelegate.Broadcast(Info_MinPhysicalAttack);

	FMageAttributeInfo Info_MaxMagicAttack = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_MaxMagicAttack, true);
	Info_MaxMagicAttack.AttributeValue = MageAttributeSet->GetMaxMagicAttack();
	AttributeInfoDelegate.Broadcast(Info_MaxMagicAttack);
	
	FMageAttributeInfo Info_MinMagicAttack = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_MinMagicAttack, true);
	Info_MinMagicAttack.AttributeValue = MageAttributeSet->GetMinMagicAttack();
	AttributeInfoDelegate.Broadcast(Info_MinMagicAttack);

	FMageAttributeInfo Info_Defense = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_Defense, true);
	Info_Defense.AttributeValue = MageAttributeSet->GetDefense();
	AttributeInfoDelegate.Broadcast(Info_Defense);

	FMageAttributeInfo Info_CriticalHitChance = AttributeInfo->FindAttributeInfoForTag(FMageGameplayTags::Get().Attribute_Secondary_CriticalHitChance, true);
	Info_CriticalHitChance.AttributeValue = MageAttributeSet->GetCriticalHitChance();
	AttributeInfoDelegate.Broadcast(Info_CriticalHitChance);
}

void UAttributeMenuWidgetController::BindCallbacks()
{
	
}
