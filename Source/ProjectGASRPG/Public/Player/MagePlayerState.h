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
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
#pragma region GAS
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override {return AbilitySystemComponent;}
	
	FORCEINLINE virtual UAttributeSet* GetAttributeSet() const {return AttributeSet;}
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
#pragma endregion

#pragma region PlayerData
public:
	FORCEINLINE int32 GetCharacterLevel() const {return Level;}
	void AddToLevel(const int32 InLevel);
	void SetLevel(const int32 InLevel);

	FORCEINLINE int32 GetExp() const {return Exp;}
	void AddToExp(const int32 InExp);
	void SetExp(const int32 InExp);

	FORCEINLINE int32 GetAttributePoint() const {return AttributePoint;}
	void AddToAttributePoint(const int32 InAttributePoint);
	void SetAttributePoint(const int32 InAttributePoint);

	FORCEINLINE int32 GetSkillPoint() const {return SkillPoint;}
	void AddToSkillPoint(const int32 InSkillPoint);
	void SetSkillPoint(const int32 InSkillPoint);

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
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Level, Category = "Mage_PlayerData|Level")
	int32 Level = 1;
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Exp, Category = "Mage_PlayerData|Level")
	int32 Exp = 0	;
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_AttributePoint, Category = "Mage_PlayerData|Level")
	int32 AttributePoint = 5;
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_SkillPoint, Category = "Mage_PlayerData|Level")
	int32 SkillPoint = 1;
	UPROPERTY(EditAnywhere,ReplicatedUsing = OnRep_CharacterClass, Category = "Mage_PlayerData|Class")
	ECharacterClass CharacterClass = ECharacterClass::Mage;

	UFUNCTION()
	virtual void OnRep_Level(const int32 OldData);
	UFUNCTION()
	virtual void OnRep_Exp(const int32 OldData);
	UFUNCTION()
	virtual void OnRep_AttributePoint(const int32 OldData);
	UFUNCTION()
	virtual void OnRep_SkillPoint(const int32 OldData);
	UFUNCTION()
	virtual void OnRep_CharacterClass(const ECharacterClass OldCharacterClass);
#pragma endregion
};
