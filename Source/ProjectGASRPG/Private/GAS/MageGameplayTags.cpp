#include "GAS/MageGameplayTags.h"
#include "GameplayTagsManager.h"

FMageGameplayTags FMageGameplayTags::GameplayTagsInstance; // 初始化静态成员变量

void FMageGameplayTags::InitNativeGameplayTags()
{
    /** AddNativeGameplayTag() 将Tag添加到引擎， GameplayTagsInstance则将Tag存储一份，放便读取 */
    
    /** Vital Attributes */
    GameplayTagsInstance.Attribute_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Health"),FString("生命值"));
    GameplayTagsInstance.Attribute_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Mana"),FString("法力值"));

    /** Primary Attributes */
    GameplayTagsInstance.Attribute_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Strength"),FString("力量"));
    GameplayTagsInstance.Attribute_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Intelligence"),FString("智力"));
    GameplayTagsInstance.Attribute_Primary_Stamina = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Stamina"),FString("体力"));
    GameplayTagsInstance.Attribute_Primary_Vigor = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Vigor"),FString("精力"));

    /** Secondary Attributes */
    GameplayTagsInstance.Attribute_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxHealth"),FString("最大生命值"));
    GameplayTagsInstance.Attribute_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMana"),FString("最大法力值"));
    GameplayTagsInstance.Attribute_Secondary_MaxPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxPhysicalAttack"),FString("最大物理攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MinPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinPhysicalAttack"),FString("最小物理攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MaxMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMagicAttack"),FString("最大魔法攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MinMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinMagicAttack"),FString("最小魔法攻击力"));
    GameplayTagsInstance.Attribute_Secondary_Defense = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.Defense"),FString("防御力"));
    GameplayTagsInstance.Attribute_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.CriticalHitChance"),FString("暴击率"));

    /** MetaAttribute */
    GameplayTagsInstance.MetaAttribute_Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("MetaAttribute.Damage"),FString("伤害值"));

    /** Input */
    GameplayTagsInstance.Input_LMB = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.LMB"),FString("鼠标左键"));
    GameplayTagsInstance.Input_Space = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Space"),FString("空格键"));
    GameplayTagsInstance.Input_Shift = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Shift"),FString("Shift键"));
    GameplayTagsInstance.Input_Tab = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Tab"),FString("Tab键"));
    GameplayTagsInstance.Input_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.1"),FString("1键"));
    GameplayTagsInstance.Input_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.2"),FString("2键"));
    GameplayTagsInstance.Input_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.3"),FString("3键"));
    GameplayTagsInstance.Input_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.4"),FString("4键"));
    GameplayTagsInstance.Input_Q = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Q"),FString("Q键"));
    GameplayTagsInstance.Input_E = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.E"),FString("E键"));
    GameplayTagsInstance.Input_R = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.R"),FString("R键"));
    GameplayTagsInstance.Input_F = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.F"),FString("F键"));
    
}
