// 


#include "Anim/AnimNotify/MageAN_PlayAttackMontageNotifyWindow.h"

#include "Character/MageCharacter.h"

void UMageAN_PlayAttackMontageNotifyWindow::BranchingPointNotifyBegin(
	FBranchingPointNotifyPayload& BranchingPointPayload)
{
	MageCharacter = Cast<AMageCharacterBase>(BranchingPointPayload.SkelMeshComponent->GetOwner());
	if(MageCharacter)
	{
		MageCharacter->SetMontageEventTag(GameplayEventTag);
		MageCharacter->AttackMontageWindowBegin();
	}
}

void UMageAN_PlayAttackMontageNotifyWindow::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if(MageCharacter)
	{
		MageCharacter->SetMontageEventTag(FGameplayTag::EmptyTag);
		MageCharacter->AttackMontageWindowEnd();
		MageCharacter = nullptr;
	}
}
