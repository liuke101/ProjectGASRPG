#include "GAS/MageGameplayTags.h"
#include "GameplayTagsManager.h"

/** 全局变量,但是由于TagsInstance是private成员,其他类访问不到。所以这里只起到初始化静态成员变量的作用（创建实例） */
FMageGameplayTags FMageGameplayTags::TagsInstance; 

void FMageGameplayTags::InitNativeGameplayTags()
{
    /** AddNativeGameplayTag() 将Tag添加到引擎， GameplayTagsInstance则将Tag存储一份，放便读取 */

    /** Character Tags*/
    TagsInstance.Character_Player = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Character.Player"),FString("玩家"));
    TagsInstance.Character_Enemy = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Character.Enemy"),FString("敌人"));
    
    /** Input */
    TagsInstance.Input_LMB = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.LMB"),FString("鼠标左键"));
    TagsInstance.Input_RMB = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.RMB"),FString("鼠标右键"));
    TagsInstance.Input_Space = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Space"),FString("空格键"));
    TagsInstance.Input_Shift = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Shift"),FString("Shift键"));
    TagsInstance.Input_Tab = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Tab"),FString("Tab键"));
    TagsInstance.Input_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.1"),FString("1键"));
    TagsInstance.Input_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.2"),FString("2键"));
    TagsInstance.Input_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.3"),FString("3键"));
    TagsInstance.Input_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.4"),FString("4键"));
    TagsInstance.Input_Q = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Q"),FString("Q键"));
    TagsInstance.Input_E = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.E"),FString("E键"));
    TagsInstance.Input_R = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.R"),FString("R键"));
    TagsInstance.Input_F = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.F"),FString("F键"));
    TagsInstance.Input_Passive1 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Passive1"),FString("被动技能1, 用作占位符来更新技能栏, 不需要绑定输入"));
    TagsInstance.Input_Passive2 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Input.Passive2"),FString("被动技能2, 用作占位符来更新技能栏, 不需要绑定输入"));
    
    /** Vital Attributes */
    TagsInstance.Attribute_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Health"),FString("生命值"));
    TagsInstance.Attribute_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Mana"),FString("法力值"));
    TagsInstance.Attribute_Vital_Vitality = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Vital.Vitality"),FString("活力值"));
   

    /** Primary Attributes */
    TagsInstance.Attribute_Primary_Strength = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Strength"),FString("力量"));
    TagsInstance.Attribute_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Intelligence"),FString("智力"));
    TagsInstance.Attribute_Primary_Stamina = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Stamina"),FString("体力"));
    TagsInstance.Attribute_Primary_Vigor = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Primary.Vigor"),FString("精力"));

    /** Secondary Attributes */
    TagsInstance.Attribute_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxHealth"),FString("最大生命值"));
    TagsInstance.Attribute_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMana"),FString("最大法力值"));
    TagsInstance.Attribute_Secondary_MaxVitality = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxVitality"),FString("最大活力值"));
    TagsInstance.Attribute_Secondary_MaxPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxPhysicalAttack"),FString("最大物理攻击力"));
    TagsInstance.Attribute_Secondary_MinPhysicalAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinPhysicalAttack"),FString("最小物理攻击力"));
    TagsInstance.Attribute_Secondary_MaxMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MaxMagicAttack"),FString("最大魔法攻击力"));
    TagsInstance.Attribute_Secondary_MinMagicAttack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.MinMagicAttack"),FString("最小魔法攻击力"));
    TagsInstance.Attribute_Secondary_Defense = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.Defense"),FString("防御力"));
    TagsInstance.Attribute_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Secondary.CriticalHitChance"),FString("暴击率"));

    /** Resistance Attributes */
    TagsInstance.Attribute_Resistance_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Fire"),FString("火抗性"));
    TagsInstance.Attribute_Resistance_Ice = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Ice"),FString("冰抗性"));
    TagsInstance.Attribute_Resistance_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Lightning"),FString("电抗性"));
    TagsInstance.Attribute_Resistance_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Resistance.Physical"),FString("物理抗性"));

    /** Meta Attributes */
    TagsInstance.Attribute_Meta_Exp = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Attribute.Meta.Exp"),FString("经验值元属性"));

    /** Damage Type */
    TagsInstance.DamageType_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Fire"),FString("火"));
    TagsInstance.DamageType_Ice = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Ice"),FString("冰"));
    TagsInstance.DamageType_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Lightning"),FString("电"));
    TagsInstance.DamageType_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("DamageType.Physical"),FString("物理"));
    // 添加到Map
    TagsInstance.DamageTypeTag_To_ResistanceTag.Add(TagsInstance.DamageType_Fire,TagsInstance.Attribute_Resistance_Fire); 
    TagsInstance.DamageTypeTag_To_ResistanceTag.Add(TagsInstance.DamageType_Ice,TagsInstance.Attribute_Resistance_Ice);
    TagsInstance.DamageTypeTag_To_ResistanceTag.Add(TagsInstance.DamageType_Lightning,TagsInstance.Attribute_Resistance_Lightning);
    TagsInstance.DamageTypeTag_To_ResistanceTag.Add(TagsInstance.DamageType_Physical,TagsInstance.Attribute_Resistance_Physical);
    
    /** Gameplay Effect */
    TagsInstance.Effects_HitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Effects.HitReact"),FString("受击反馈时作为Added Granted Tag"));

    /** Gameplay Ability */
    TagsInstance.Ability_None = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.None"),FString("无技能"));
    TagsInstance.Ability_Attack = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Attack"),FString("攻击"));
    TagsInstance.Ability_Summon = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Summon"),FString("召唤"));
    TagsInstance.Ability_HitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.HitReact"),FString("受击反馈"));
    TagsInstance.Ability_Mage_Fire_Fireball = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Mage.Fire.Fireball"),FString("法师-火-火球术"));
    TagsInstance.Cooldown_Mage_Fire_Fireball = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Cooldown.Mage.Fire.Fireball"),FString("法师-火-火球术冷却"));
    TagsInstance.Ability_Mage_Lightning_Laser = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Mage.Lightning.Laser"),FString("法师-电-镭射"));
    TagsInstance.Cooldown_Mage_Lightning_Laser = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Cooldown.Mage.Lightning.Laser"),FString("法师-电-镭射冷却"));

    /** Passive Ability */
    TagsInstance.Ability_Passive_ProtectiveHalo = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Passive.ProtectiveHalo"),FString("被动-保护光环"));
    TagsInstance.Ability_Passive_HealthSiphon = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Passive.HealthSiphon"),FString("被动-生命虹吸"));
    TagsInstance.Ability_Passive_ManaSiphon = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Passive.ManaSiphon"),FString("被动-法术虹吸"));

    /** Ability State */
    TagsInstance.Ability_State_Locked = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.State.Locked"),FString("未解锁"));
    TagsInstance.Ability_State_Trainable = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.State.Trainable"),FString("可学习"));
    TagsInstance.Ability_State_Unlocked = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.State.Unlocked"),FString("已解锁"));
    TagsInstance.Ability_State_Equipped = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.State.Equipped"),FString("已装备"));

    /** Ability Type */
    TagsInstance.Ability_Type_Active = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Type.Active"),FString("主动技能"));
    TagsInstance.Ability_Type_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Type.Passive"),FString("被动技能"));
    TagsInstance.Ability_Type_None = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Ability.Type.None"),FString("无类型"));

    /** Debuff */
    TagsInstance.Debuff_Type_Burn = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Type.Burn"),FString("灼烧"));
    TagsInstance.Debuff_Type_Frozen = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Type.Frozen"),FString("冰冻"));
    TagsInstance.Debuff_Type_Stun = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Type.Stun"),FString("眩晕"));
    TagsInstance.Debuff_Type_Bleed = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Type.Bleed"),FString("流血"));
    
    // 添加到Map
    TagsInstance.DamageTypeTag_To_DebuffTag.Add(TagsInstance.DamageType_Fire,TagsInstance.Debuff_Type_Burn);
    TagsInstance.DamageTypeTag_To_DebuffTag.Add(TagsInstance.DamageType_Ice,TagsInstance.Debuff_Type_Frozen);
    TagsInstance.DamageTypeTag_To_DebuffTag.Add(TagsInstance.DamageType_Lightning,TagsInstance.Debuff_Type_Stun);
    TagsInstance.DamageTypeTag_To_DebuffTag.Add(TagsInstance.DamageType_Physical,TagsInstance.Debuff_Type_Bleed);

    TagsInstance.Debuff_Params_Chance = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Params.Chance"),FString("触发几率"));
    TagsInstance.Debuff_Params_Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Params.Damage"),FString("伤害"));
    TagsInstance.Debuff_Params_Frequency = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Params.Frequency"),FString("频率"));
    TagsInstance.Debuff_Params_Duration = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Debuff.Params.Duration"),FString("持续时间"));

    /** Montage */
    TagsInstance.AttackSocket_Weapon = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.Weapon")),FString("使用武器攻击");
    TagsInstance.AttackSocket_LeftHand = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.LeftHand")),FString("使用左手攻击");
    TagsInstance.AttackSocket_RightHand = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.RightHand")),FString("使用右手攻击");
    TagsInstance.AttackSocket_Tail = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("AttackSocket.Tail")),FString("使用尾巴攻击");

    TagsInstance.Montage_Attack_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.1"),FString("攻击动画Montage1"));
    TagsInstance.Montage_Attack_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.2"),FString("攻击动画Montage2"));
    TagsInstance.Montage_Attack_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.3"),FString("攻击动画Montage3"));
    TagsInstance.Montage_Attack_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Montage.Attack.4"),FString("攻击动画Montage4"));


    /** Player Controller */
    TagsInstance.Player_Block_InputPressed = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Player.Block.InputPressed"),FString("阻止InputPressed"));
    TagsInstance.Player_Block_InputHold = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Player.Block.InputHold"),FString("阻止InputHold"));
    TagsInstance.Player_Block_InputReleased = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Player.Block.InputReleased"),FString("阻止InputReleased"));
    TagsInstance.Player_Block_CursorTrace = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Player.Block.CursorTrace"),FString("阻止CursorTrace"));
}
