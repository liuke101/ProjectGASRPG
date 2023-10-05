// 

#pragma once

#include "CoreMinimal.h"
#include "GammerCharacterBase.h"
#include "GammerCharacter.generated.h"

UCLASS()
class PROJECTGASRPG_API AGammerCharacter : public AGammerCharacterBase
{
	GENERATED_BODY()

public:
	AGammerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
