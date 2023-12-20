// 


#include "Anim/AnimNotify/MageAN_PlayAttackMontageNotifyWindow.h"

#include "Character/MageCharacter.h"

UMageAN_PlayAttackMontageNotifyWindow::UMageAN_PlayAttackMontageNotifyWindow(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//默认通知名称
	NotifyName = TEXT("MontageWindow");
}

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
