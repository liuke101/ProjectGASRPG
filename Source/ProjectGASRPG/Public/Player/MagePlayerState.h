#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "MagePlayerState.generated.h"

class ULevelDataAsset;
class UAttributeSet;
class UAbilitySystemComponent;

/** 玩家数据变化委托 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerDataChangedDelegate, int32 /*DataValue*/);

UCLASS()
class PROJECTGASRPG_API AMagePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AMagePlayerState();
protected:
#pragma region GAS
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override {return AbilitySystemComponent;}
	
	FORCEINLINE virtual UAttributeSet* GetAttributeSet() const {return AttributeSet;}
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MagePlayer|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MagePlayer|GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
#pragma endregion

#pragma region PlayerData
public:
	FORCEINLINE int32 GetCharacterLevel() const {return Level;}
	void SetLevel(const int32 InLevel);
	void AddToLevel(const int32 InLevel);

	FORCEINLINE int32 GetExp() const {return Exp;}
	void SetExp(const int32 InExp);
	void AddToExp(const int32 InExp);

	FORCEINLINE int32 GetAttributePoint() const {return AttributePoint;}
	void SetAttributePoint(const int32 InAttributePoint);
	void AddToAttributePoint(const int32 InAttributePoint);
	
	FORCEINLINE int32 GetSkillPoint() const {return SkillPoint;}
	void SetSkillPoint(const int32 InSkillPoint);
	void AddToSkillPoint(const int32 InSkillPoint);

	FORCEINLINE ECharacterClass GetCharacterClass() const {return CharacterClass;}

	/** 声明委托 */
	FOnPlayerDataChangedDelegate OnPlayerLevelChanged; // 等级发生变化时广播
	FOnPlayerDataChangedDelegate OnPlayerExpChanged; //经验发生变化时广播
	FOnPlayerDataChangedDelegate OnPlayerAttributePointChanged; // 属性点发生变化时广播
	FOnPlayerDataChangedDelegate OnPlayerSkillPointChanged; // 技能点发生变化时广播

	/** DataAsset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_PlayerData|Level")
	TObjectPtr<ULevelDataAsset> LevelDataAsset;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_PlayerData|Level", meta = (AllowPrivateAccess = "true"))
	int32 Level = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_PlayerData|Level", meta = (AllowPrivateAccess = "true"))
	int32 Exp = 0	;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_PlayerData|Level", meta = (AllowPrivateAccess = "true"))
	int32 AttributePoint = 5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_PlayerData|Level", meta = (AllowPrivateAccess = "true"))
	int32 SkillPoint = 1;
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Mage_PlayerData|Class", meta = (AllowPrivateAccess = "true"))
	ECharacterClass CharacterClass = ECharacterClass::Mage;

#pragma endregion
};
