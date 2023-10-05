#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "MageEnemy.generated.h"

UCLASS()
class PROJECTGASRPG_API AMageEnemy : public AMageCharacterBase
{
	GENERATED_BODY()

public:
	AMageEnemy();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


};
