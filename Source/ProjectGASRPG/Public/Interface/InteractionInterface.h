// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

UINTERFACE()
class UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTGASRPG_API IInteractionInterface
{
	GENERATED_BODY()

public:
	virtual void HighlightActor() = 0;
	virtual void UnHighlightActor() = 0;
};
