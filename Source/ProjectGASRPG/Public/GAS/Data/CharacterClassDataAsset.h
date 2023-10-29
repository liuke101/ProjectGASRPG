﻿// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterClassDataAsset.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/** 角色类型 */
UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	None,
	Warrior,
	Mage,
	Ranger
};

/** 角色默认信息(类型特有的信息) */
USTRUCT(BlueprintType)
struct FCharacterClassDefaultInfo
{
	GENERATED_BODY()
	
	/** 主要属性 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TSubclassOf<UGameplayEffect> PrimaryAttribute;

	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TArray<TSubclassOf<UGameplayAbility>> BaseAbilities;
};

/**
 *	角色类型信息DataAsset, 存储在GameMode
 */
UCLASS()
class PROJECTGASRPG_API UCharacterClassDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 每个类型型单独设置 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TMap<ECharacterClass, FCharacterClassDefaultInfo> CharacterClassDefaultInfos;

	/** 所有类型共享 */
	/** Vital Attributes 只是设置了初始血量为最大值 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TSubclassOf<UGameplayEffect> VitalAttribute;

	/** TODO:抗性属性暂时简单给个值 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TSubclassOf<UGameplayEffect> ResistanceAttribute;

	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TArray<TSubclassOf<UGameplayAbility>> CommonAbilities;

	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TObjectPtr<UCurveTable> CalcDamageCurveTable;
	
	/** 根据角色类型获取角色默认信息 */
	FCharacterClassDefaultInfo GetClassDefaultInfo(ECharacterClass CharacterClass);
};
