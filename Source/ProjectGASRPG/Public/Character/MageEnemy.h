﻿#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "MageEnemy.generated.h"

class UBehaviorTree;
class AMageAIController;
enum class ECharacterClass : uint8;
class UWidgetComponent;

UCLASS()
class PROJECTGASRPG_API AMageEnemy : public AMageCharacterBase, public IEnemyInterface
{
	GENERATED_BODY()

public:
	AMageEnemy();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region EnemyInterface
public:
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;

	FORCEINLINE virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override { CombatTarget = InCombatTarget; }
	FORCEINLINE virtual AActor* GetCombatTarget_Implementation() const override { return CombatTarget; }
	
	UPROPERTY(BlueprintReadWrite, Category = "MageCharacter|EnemyInterface")
	TObjectPtr<AActor> CombatTarget;
#pragma endregion

#pragma region CombatInterface
public:
	FORCEINLINE virtual int32 GetCharacterLevel() const override { return Level; }
	FORCEINLINE virtual ECharacterClass GetCharacterClass() const override { return CharacterClass; }
	
	virtual void Die(const FVector& DeathImpulse) override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageCharacter|CombatInterface")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageCharacter|CombatInterface")
	float LifeSpan = 5.0f;

	/** 角色类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageCharacter|CombatInterface")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;

	
#pragma endregion
	
#pragma region GAS
public: 
	virtual void InitASC() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void StunTagChanged(const FGameplayTag CallbackTag, const int32 NewCount) override;
protected:
	virtual void InitDefaultAttributes() const override;
	
	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UPROPERTY(BlueprintReadOnly, Category = "MageCharacter|GAS")
	bool bHitReacting;
#pragma endregion

#pragma region UI
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MageCharacter|UI")
	TObjectPtr<UWidgetComponent> HealthBar;
#pragma endregion

#pragma region AI
protected:
	UPROPERTY()
	TObjectPtr<AMageAIController> MageAIController;

	UPROPERTY(EditAnywhere, Category = "MageCharacter|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

#pragma endregion
};
