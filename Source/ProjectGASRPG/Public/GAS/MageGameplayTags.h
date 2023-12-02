#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
  * 饿汉式单例模式
  * 
  * 访问GameplayTag的方法：
  * 1. 获取单例：const FMageGameplayTags& GameplayTagsInstance = FMageGameplayTags::Instance();
  * 2. 通过单例访问FGameplayTag成员变量：GameplayTagsInstance.Attribute_Secondary_MaxHealth
  */
class FMageGameplayTags
{
public:
	static const FMageGameplayTags& Instance() { return TagsInstance; } // 单例
	
	/* 添加C++Native(原生) GameplayTag, 不再使用ini或数据表配置Tag */
	static void InitNativeGameplayTags();
	
#pragma region "GameplayTag"
	/** Character Tags*/
	FGameplayTag Character_Player;
	FGameplayTag Character_Enemy;
	
	/** Input */
	FGameplayTag Input_LMB;
	FGameplayTag Input_RMB;
	FGameplayTag Input_Space;
	FGameplayTag Input_Shift;
	FGameplayTag Input_Tab;
	FGameplayTag Input_1;
	FGameplayTag Input_2;
	FGameplayTag Input_3;
	FGameplayTag Input_4;
	FGameplayTag Input_Q;
	FGameplayTag Input_E;
	FGameplayTag Input_R;
	FGameplayTag Input_F;
	FGameplayTag Input_Passive1;
	FGameplayTag Input_Passive2;
	
	/** Vital Attributes */
	FGameplayTag Attribute_Vital_Health;
	FGameplayTag Attribute_Vital_Mana;
	FGameplayTag Attribute_Vital_Vitality;
	

	/** Primary Attributes */
	FGameplayTag Attribute_Primary_Strength;
	FGameplayTag Attribute_Primary_Intelligence;
	FGameplayTag Attribute_Primary_Stamina;
	FGameplayTag Attribute_Primary_Vigor;

	/** Secondary Attributes */
	FGameplayTag Attribute_Secondary_MaxHealth;
	FGameplayTag Attribute_Secondary_MaxMana;
	FGameplayTag Attribute_Secondary_MaxVitality;
	FGameplayTag Attribute_Secondary_MaxPhysicalAttack;
	FGameplayTag Attribute_Secondary_MinPhysicalAttack;
	FGameplayTag Attribute_Secondary_MaxMagicAttack;
	FGameplayTag Attribute_Secondary_MinMagicAttack;
	FGameplayTag Attribute_Secondary_Defense;
	FGameplayTag Attribute_Secondary_CriticalHitChance;

	/** Resistance Attributes */
	FGameplayTag Attribute_Resistance_Fire;
	FGameplayTag Attribute_Resistance_Ice;
	FGameplayTag Attribute_Resistance_Lightning;
	FGameplayTag Attribute_Resistance_Physical;

	/** Meta Attributes */
	FGameplayTag Attribute_Meta_Exp; // 经验值元属性

	/** Damage Type */
	TMap<FGameplayTag,FGameplayTag> DamageTypeTag_To_ResistanceTag; // 伤害类型与抗性的映射
	FGameplayTag DamageType_Fire;
	FGameplayTag DamageType_Ice;
	FGameplayTag DamageType_Lightning; 
	FGameplayTag DamageType_Physical;

	/** Debuff*/
	TMap<FGameplayTag,FGameplayTag> DamageTypeTag_To_DebuffTag; // 伤害类型与Debuff的映射
	FGameplayTag Debuff_Type_Burn;
	FGameplayTag Debuff_Type_Frozen;
	FGameplayTag Debuff_Type_Stun;
	FGameplayTag Debuff_Type_Bleed;
	
	FGameplayTag Debuff_Params_Chance;
	FGameplayTag Debuff_Params_Damage;
	FGameplayTag Debuff_Params_Frequency;
	FGameplayTag Debuff_Params_Duration;
	
	
	/** Gameplay Effect */
	FGameplayTag Effects_HitReact;

	/** Gameplay Ability */
	/** Active Ability */
	FGameplayTag Ability_None;	
	FGameplayTag Ability_Attack;
	FGameplayTag Ability_Summon;
	FGameplayTag Ability_HitReact;
	
	FGameplayTag Ability_Mage_Fire_Fireball;
	FGameplayTag Cooldown_Mage_Fire_Fireball;
	
	FGameplayTag Ability_Mage_Lightning_Laser;
	FGameplayTag Cooldown_Mage_Lightning_Laser;
	
	/** Passive Ability */
	FGameplayTag Ability_Passive_ProtectiveHalo; // 保护光环
	FGameplayTag Ability_Passive_HealthSiphon; // 生命虹吸
	FGameplayTag Ability_Passive_ManaSiphon; // 法力虹吸

	/** Ability State */
	FGameplayTag Ability_State_Locked;   // 未解锁
	FGameplayTag Ability_State_Trainable; // 可学的（学习条件达标，比如等级、属性达标）
	FGameplayTag Ability_State_Unlocked; // 已解锁
	FGameplayTag Ability_State_Equipped; // 已装备

	/** Ability Type */ 
	FGameplayTag Ability_Type_Active; // 主动技能
	FGameplayTag Ability_Type_Passive; // 被动技能
	FGameplayTag Ability_Type_None; // 无类型

	/** AttackSocket(用于攻击命中判定) */
	FGameplayTag AttackSocket_Weapon;
	FGameplayTag AttackSocket_LeftHand;
	FGameplayTag AttackSocket_RightHand;
	FGameplayTag AttackSocket_Tail;

	/** Montage */
	FGameplayTag Montage_Attack_1;
	FGameplayTag Montage_Attack_2;
	FGameplayTag Montage_Attack_3;
	FGameplayTag Montage_Attack_4;

	/**
	 * Player Controller
	 * - 在GA的 Activation Owned Tags 中配置, 来控制输入状态
	 */
	FGameplayTag Player_Block_InputPressed;
	FGameplayTag Player_Block_InputHold;
	FGameplayTag Player_Block_InputReleased;
	FGameplayTag Player_Block_CursorTrace;

#pragma endregion

private:
	static FMageGameplayTags TagsInstance;
};




