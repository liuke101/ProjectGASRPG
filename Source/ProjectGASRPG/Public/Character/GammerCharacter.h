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
	// Sets default values for this character's properties
	AGammerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
