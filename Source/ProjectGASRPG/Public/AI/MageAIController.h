#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MageAIController.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class PROJECTGASRPG_API AMageAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMageAIController();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
#pragma region AI
	
	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;
#pragma endregion
};
