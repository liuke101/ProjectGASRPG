// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "GameplayTagInterface.generated.h"

UINTERFACE()
class UGameplayTagInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTGASRPG_API IGameplayTagInterface
{
	GENERATED_BODY()

public:
	virtual FGameplayTagContainer GetGameplayTags() const = 0;
};
