#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
  * 单例类
  * 
  * 访问GameplayTag的方法：
  * 1. 获取单例：const FMageGameplayTags& GameplayTagsInstance = FMageGameplayTags::Get();
  * 2. 通过单例访问FGameplayTag成员变量：GameplayTagsInstance.Attribute_Secondary_MaxHealth
  */
class FMageGameplayTags
{
public:
	static const FMageGameplayTags& Get() { return GameplayTagsInstance; } // 单例

	/* 添加C++Native(原生) GameplayTag, 不再使用ini或数据表配置Tag */
	static void InitNativeGameplayTags();
	
#pragma region "GameplayTags"
	/** Character Tags*/
	FGameplayTag Character_Player;
	FGameplayTag Character_Enemy;
	
	/** Input */
	FGameplayTag Input_LMB;
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
	
	/** Vital Attributes */
	FGameplayTag Attribute_Vital_Health;
	FGameplayTag Attribute_Vital_Mana;

	/** Primary Attributes */
	FGameplayTag Attribute_Primary_Strength;
	FGameplayTag Attribute_Primary_Intelligence;
	FGameplayTag Attribute_Primary_Stamina;
	FGameplayTag Attribute_Primary_Vigor;

	/** Secondary Attributes */
	FGameplayTag Attribute_Secondary_MaxHealth;
	FGameplayTag Attribute_Secondary_MaxMana;
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

	/** SetByCaller */
	FGameplayTag SetByCaller_Damage;

	/** Damage Type */
	TMap<FGameplayTag,FGameplayTag> DamageTypeTag_To_ResistanceTag; // 伤害类型与抗性的映射
	FGameplayTag DamageType_Fire;
	FGameplayTag DamageType_Ice;
	FGameplayTag DamageType_Lightning; 
	FGameplayTag DamageType_Physical;
	
	/** Gameplay Effect */
	FGameplayTag Effects_HitReact;

	/** Gameplay Ability */
	FGameplayTag Ability_Mage_Fireball;
	FGameplayTag Ability_Attack;

	/** Montage */
	FGameplayTag Montage_Attack_Weapon;
	FGameplayTag Montage_Attack_LeftHand;
	FGameplayTag Montage_Attack_RightHand;

#pragma endregion

private:
	static FMageGameplayTags GameplayTagsInstance;
};


