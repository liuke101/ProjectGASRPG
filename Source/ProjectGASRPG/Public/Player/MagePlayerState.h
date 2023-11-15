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
	
	FORCEINLINE int32 GetCharacterLevel() const {return Level;}
	void AddToLevel(int32 InLevel);
	void SetLevel(int32 InLevel);

	FORCEINLINE int32 GetExp() const {return Exp;}
	void AddToExp(int32 InExp);
	void SetExp(int32 InExp);

	FORCEINLINE ECharacterClass GetCharacterClass() const {return CharacterClass;}

	/** 等级发生变化时广播 */
	FOnPlayerDataChangedDelegate OnPlayerLevelChanged;

	/** 经验发生变化时广播 */
	FOnPlayerDataChangedDelegate OnPlayerExpChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<ULevelDataAsset> LevelDataAsset;
	

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<UAttributeSet> AttributeSet;

private:
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Level, Category = "Mage_GAS")
	int32 Level = 1;

	UFUNCTION()
	virtual void OnRep_Level(int32 OldData);
	
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_EXP, Category = "Mage_GAS")
	int32 Exp;

	UFUNCTION()
	virtual void OnRep_EXP(int32 OldData);

	UPROPERTY(EditAnywhere,ReplicatedUsing = OnRep_CharacterClass, Category = "Mage_GAS")
	ECharacterClass CharacterClass = ECharacterClass::Mage;

	UFUNCTION()
	virtual void OnRep_CharacterClass(ECharacterClass OldCharacterClass);
#pragma endregion
};
