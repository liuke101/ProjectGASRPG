#include "GAS/MageGameplayTags.h"

#include "AIController.h"
#include "GameplayTagsManager.h"

FMageGameplayTags FMageGameplayTags::GameplayTagsInstance; // 初始化静态成员变量

void FMageGameplayTags::InitNativeGameplayTags()
{
    /** AddNativeGameplayTag() 将Tag添加到引擎， GameplayTagsInstance则将Tag存储一份，放便读取 */

    /** Character Tags*/
    GameplayTagsInstance.Character_Player = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Character.Player"),FString("玩家"));
    GameplayTagsInstance.Character_Enemy = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Character.Enemy"),FString("敌人"));
    
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
    
    /** Vital Attributes */
    GameplayTagsInstance.Attribute_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Health"),FString("生命值"));
    GameplayTagsInstance.Attribute_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Mana"),FString("法力值"));
    GameplayTagsInstance.Attribute_Vital_Vitality = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Vitality"),FString("活力值"));
   

    /** Primary Attributes */
    GameplayTagsInstance.Attribute_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Strength"),FString("力量"));
    GameplayTagsInstance.Attribute_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Intelligence"),FString("智力"));
    GameplayTagsInstance.Attribute_Primary_Stamina = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Stamina"),FString("体力"));
    GameplayTagsInstance.Attribute_Primary_Vigor = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Vigor"),FString("精力"));

    /** Secondary Attributes */
    GameplayTagsInstance.Attribute_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxHealth"),FString("最大生命值"));
    GameplayTagsInstance.Attribute_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMana"),FString("最大法力值"));
    GameplayTagsInstance.Attribute_Secondary_MaxVitality = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxVitality"),FString("最大活力值"));
    GameplayTagsInstance.Attribute_Secondary_MaxPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxPhysicalAttack"),FString("最大物理攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MinPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinPhysicalAttack"),FString("最小物理攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MaxMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMagicAttack"),FString("最大魔法攻击力"));
    GameplayTagsInstance.Attribute_Secondary_MinMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinMagicAttack"),FString("最小魔法攻击力"));
    GameplayTagsInstance.Attribute_Secondary_Defense = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.Defense"),FString("防御力"));
    GameplayTagsInstance.Attribute_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.CriticalHitChance"),FString("暴击率"));

    /** Resistance Attributes */
    GameplayTagsInstance.Attribute_Resistance_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Fire"),FString("火抗性"));
    GameplayTagsInstance.Attribute_Resistance_Ice = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Ice"),FString("冰抗性"));
    GameplayTagsInstance.Attribute_Resistance_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Lightning"),FString("电抗性"));
    GameplayTagsInstance.Attribute_Resistance_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Physical"),FString("物理抗性"));
    
    /** SetByCaller */
    GameplayTagsInstance.SetByCaller_Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("SetByCaller.Damage"),FString("伤害值"));

    /** Damage Type */
    GameplayTagsInstance.DamageType_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Fire"),FString("火"));
    GameplayTagsInstance.DamageType_Ice = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Ice"),FString("冰"));
    GameplayTagsInstance.DamageType_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Lightning"),FString("电"));
    GameplayTagsInstance.DamageType_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Physical"),FString("物理"));
    // 添加到Map
    GameplayTagsInstance.DamageTypeTag_To_ResistanceTag.Add(GameplayTagsInstance.DamageType_Fire,GameplayTagsInstance.Attribute_Resistance_Fire); 
    GameplayTagsInstance.DamageTypeTag_To_ResistanceTag.Add(GameplayTagsInstance.DamageType_Ice,GameplayTagsInstance.Attribute_Resistance_Ice);
    GameplayTagsInstance.DamageTypeTag_To_ResistanceTag.Add(GameplayTagsInstance.DamageType_Lightning,GameplayTagsInstance.Attribute_Resistance_Lightning);
    GameplayTagsInstance.DamageTypeTag_To_ResistanceTag.Add(GameplayTagsInstance.DamageType_Physical,GameplayTagsInstance.Attribute_Resistance_Physical);
    
    /** Gameplay Effect */
    GameplayTagsInstance.Effects_HitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Effects.HitReact"),FString("受击反馈时作为Added Granted Tag"));

    /** Gameplay Ability */
    GameplayTagsInstance.Ability_Attack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Attack"),FString("攻击"));
    GameplayTagsInstance.Ability_Summon = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Summon"),FString("召唤"));
    GameplayTagsInstance.Ability_Mage_Fireball = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Mage.Fireball"),FString("法师-火球术"));

    /** Montage */
    GameplayTagsInstance.AttackSocket_Weapon = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.Weapon")),FString("使用武器攻击");
    GameplayTagsInstance.AttackSocket_LeftHand = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.LeftHand")),FString("使用左手攻击");
    GameplayTagsInstance.AttackSocket_RightHand = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.RightHand")),FString("使用右手攻击");
    GameplayTagsInstance.AttackSocket_Tail = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.Tail")),FString("使用尾巴攻击");

    GameplayTagsInstance.Montage_Attack_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.1"),FString("攻击动画Montage1"));
    GameplayTagsInstance.Montage_Attack_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.2"),FString("攻击动画Montage2"));
    GameplayTagsInstance.Montage_Attack_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.3"),FString("攻击动画Montage3"));
    GameplayTagsInstance.Montage_Attack_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.4"),FString("攻击动画Montage4"));
}
