#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"
UINTERFACE()
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTGASRPG_API ICombatInterface
{
	GENERATED_BODY()

public:
	FORCEINLINE virtual int32 GetPlayerLevel() {return 0;}
};
