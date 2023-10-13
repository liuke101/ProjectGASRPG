#include "GAS/MageGameplayTags.h"

#include "GameplayTagsManager.h"

FMageGameplayTags FMageGameplayTags::Instance;

void FMageGameplayTags::InitNativeGameplayTags()
{
	UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxHealthTest"),FString("最大生命值"));
}
