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

	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_Enemy")
	bool bIsHighlighted = false;

};
