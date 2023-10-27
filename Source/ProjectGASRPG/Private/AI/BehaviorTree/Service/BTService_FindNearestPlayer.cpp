#include "AI/BehaviorTree/Service/BTService_FindNearestPlayer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Character/MageCharacterBase.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"

void UBTService_FindNearestPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* OwningPawn = AIOwner->GetPawn();  //获取AIController控制的Pawn

	if(const IGameplayTagInterface* GameplayTagInterface = Cast<IGameplayTagInterface>(OwningPawn))
	{
		const FMageGameplayTags& Tags = FMageGameplayTags::Get();
		//该 Pawn 的 Tag 如果是 Character_Enemy, 则 TargetTag 为 Character_Player
		const FGameplayTag TargetTag = GameplayTagInterface->GetGameplayTags().HasTagExact(Tags.Character_Enemy) ? Tags.Character_Player : Tags.Character_Enemy;

		TArray<AActor*> ActorsWithTag;
		UMageAbilitySystemLibrary::GetAllActorsWithGameplayTag(OwningPawn, TargetTag, ActorsWithTag, true);


		
	}
}
