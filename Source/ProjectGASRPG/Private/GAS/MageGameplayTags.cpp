#include "GAS/MageGameplayTags.h"
#include "GameplayTagsManager.h"

FMageGameplayTags FMageGameplayTags::GameplayTagsInstance; // 初始化静态成员变量

void FMageGameplayTags::InitNativeGameplayTags()
{
    GameplayTagsInstance.Attribute_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxHealth"),FString("最大生命值"));
}
