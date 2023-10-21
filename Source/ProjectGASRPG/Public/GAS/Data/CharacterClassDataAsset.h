// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterClassDataAsset.generated.h"

class UGameplayEffect;

/** 角色类型 */
UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	Warrior,
	Mage,
	Ranger
};

/** 角色默认信息 */
USTRUCT(BlueprintType)
struct FCharacterClassDefaultInfo
{
	GENERATED_BODY()

	
	/** 主要属性 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TSubclassOf<UGameplayEffect> PrimaryAttribute;

	//TODO:暂时让所有类型都用相同的MMC计算SecondaryAttributes, 后续要为每个类型设置独立的MMC
	/** 次要属性 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TSubclassOf<UGameplayEffect> SecondaryAttribute;
};

/**
 *	角色类型信息DataAsset, 存储在GameMode
 */
UCLASS()
class PROJECTGASRPG_API UCharacterClassDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TMap<ECharacterClass, FCharacterClassDefaultInfo> CharacterClassDefaultInfos;

	/** Vital Attributes 只是设置了初始血量为最大值，所有角色类型共享 */
	UPROPERTY(EditDefaultsOnly,Category = "GAS_CharacterClassInfo")
	TSubclassOf<UGameplayEffect> VitalAttribute;


	/** 根据角色类型获取角色默认信息 */
	FCharacterClassDefaultInfo GetClassDefaultInfo(ECharacterClass CharacterClass);
};
