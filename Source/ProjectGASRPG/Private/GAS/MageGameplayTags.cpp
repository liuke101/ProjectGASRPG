#include "GAS/MageGameplayTags.h"
#include "GameplayTagsManager.h"

FMageGameplayTags FMageGameplayTags::GameplayTagsInstance; // 初始化静态成员变量

void FMageGameplayTags::InitNativeGameplayTags()
{
    /* AddNativeGameplayTag 将Tag添加到引擎， GameplayTagsInstance则将Tag存储一份，放便读取 */
    
    /* Vital Attributes */
    GameplayTagsInstance.Attribute_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Health"),FString("生命值"));
    GameplayTagsInstance.Attribute_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Mana"),FString("法力值"));

    /* Primary Attributes */
    GameplayTagsInstance.Attribute_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Strength"),FString("力量"));
    GameplayTagsInstance.Attribute_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Intelligence"),FString("智力"));
    GameplayTagsInstance.Attribute_Primary_Stamina = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Stamina"),FString("体力"));
    GameplayTagsInstance.Attribute_Primary_Vigor = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Vigor"),FString("精力"));

    /* Secondary Attributes */
    GameplayTagsInstance.Attribute_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxHealth"),FString("最大生命值"));
    GameplayTagsInstance.Attribute_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMana"),FString("最大法力值"));
    GameplayTagsInstance.Attribute_Secondary_MaxPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxPhysicalAttack"),FString("最大物理攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MinPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinPhysicalAttack"),FString("最小物理攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MaxMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMagicAttack"),FString("最大魔法攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MinMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinMagicAttack"),FString("最小魔法攻击力"));
    GameplayTagsInstance.Attribute_Secondary_Defense = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.Defense"),FString("防御力"));
    GameplayTagsInstance.Attribute_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.CriticalHitChance"),FString("暴击率"));
}
