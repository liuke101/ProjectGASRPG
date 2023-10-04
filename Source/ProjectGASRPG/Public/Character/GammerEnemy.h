#pragma once

#include "CoreMinimal.h"
#include "GammerCharacterBase.h"
#include "GammerEnemy.generated.h"

UCLASS()
class PROJECTGASRPG_API AGammerEnemy : public AGammerCharacterBase
{
	GENERATED_BODY()

public:
	AGammerEnemy();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
};
