// 


#include "Anim/AnimNotify/MageAN_PlayAttackMontageNotifyWindow.h"

#include "Character/MageCharacter.h"

void UMageAN_PlayAttackMontageNotifyWindow::BranchingPointNotifyBegin(
	FBranchingPointNotifyPayload& BranchingPointPayload)
{
	MageCharacter = Cast<AMageCharacter>(BranchingPointPayload.SkelMeshComponent->GetOwner());
	if(MageCharacter)
	{
		MageCharacter->AttackMontageWindowBegin();
	}
}

void UMageAN_PlayAttackMontageNotifyWindow::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if(MageCharacter)
	{
		MageCharacter->AttackMontageWindowEnd();
		MageCharacter = nullptr;
	}
}
