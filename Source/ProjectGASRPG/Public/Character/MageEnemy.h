#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "Interface/EnemyInterface.h"
#include "MageEnemy.generated.h"

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
#pragma endregion

#pragma region CombatInterface
public:
	FORCEINLINE virtual int32 GetPlayerLevel() override { return Level; }
#pragma endregion
	
#pragma region ASC

public: 
	virtual void InitAbilityActorInfo() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	int32 Level = 1;
#pragma endregion
};
