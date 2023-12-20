#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlueprintBase.h"
#include "BTService_FindNearestPlayer.generated.h"

/** 服务节点，寻找最近的玩家 */
//在界面中隐藏
UCLASS()
class PROJECTGASRPG_API UBTService_FindNearestPlayer : public UBTService_BlueprintBase
{
	GENERATED_BODY()

protected:
	/** 要在蓝图中重载 Event Receive Tick AI 才会执行该函数 */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	

	/** 用于关联 Key, 通过Selector可以访问Key并修改值 */
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category= "Mage_BlackboardKey")
	FBlackboardKeySelector TargetToFollowSelector;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category= "Mage_BlackboardKey")
	FBlackboardKeySelector DistanceToTargetSelector;
	
	
};
