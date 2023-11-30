#include "AI/MageAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

AMageAIController::AMageAIController()
{
	Blackboard = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	checkf(BehaviorTreeComponent, TEXT("BehaviorTreeComponent为空, 请在BP_MageAIController中设置"));
}

void AMageAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AMageAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

