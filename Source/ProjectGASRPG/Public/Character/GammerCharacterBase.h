#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GammerCharacterBase.generated.h"

UCLASS(Abstract)
class PROJECTGASRPG_API AGammerCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AGammerCharacterBase();

protected:
	virtual void BeginPlay() override;
};
