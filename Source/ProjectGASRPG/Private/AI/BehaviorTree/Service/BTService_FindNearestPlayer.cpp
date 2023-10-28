#include "AI/BehaviorTree/Service/BTService_FindNearestPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BTFunctionLibrary.h"
#include "Component/GameplayTagsComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"

void UBTService_FindNearestPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* OwningPawn = AIOwner->GetPawn();  //获取AIController控制的Pawn
	
	if(const UGameplayTagsComponent* GameplayTagsComponent = OwningPawn->FindComponentByClass<UGameplayTagsComponent>())
	{
		/** 获取所有Tag为 Character_Player 的 Actor */
		const FMageGameplayTags& Tags = FMageGameplayTags::Get();
		//该 Pawn 的 Tag 如果是 Character_Enemy, 则 TargetTag 为 Character_Player
		const FGameplayTag TargetTag = GameplayTagsComponent->GetGameplayTags().HasTagExact(Tags.Character_Enemy) ? Tags.Character_Player : Tags.Character_Enemy;

		TArray<AActor*> ActorsWithTag;
		UMageAbilitySystemLibrary::GetAllActorsWithGameplayTag(OwningPawn, TargetTag, ActorsWithTag, true);

		/** 找到最近的Actor, 计算最近距离 */
		float NearestDistance = TNumericLimits<float>::Max();
		AActor* NearestActor = nullptr;
		for(AActor* Actor : ActorsWithTag)
		{
			if(IsValid(Actor))
			{
				const float Distance = OwningPawn->GetDistanceTo(Actor);
				if(Distance < NearestDistance)
				{
					NearestDistance = Distance;
					NearestActor = Actor;
				}
			}
		}

		/** 设置黑板值 */
		UBTFunctionLibrary::SetBlackboardValueAsObject(this, TargetToFollowSelector, NearestActor);
		UBTFunctionLibrary::SetBlackboardValueAsFloat(this, DistanceToTargetSelector, NearestDistance);
	}
}
