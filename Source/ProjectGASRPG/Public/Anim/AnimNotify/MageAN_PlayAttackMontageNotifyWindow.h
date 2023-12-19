﻿// 

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphRuntime/Public/AnimNotifies/AnimNotify_PlayMontageNotify.h"
#include "MageAN_PlayAttackMontageNotifyWindow.generated.h"

class AMageCharacter;
/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMageAN_PlayAttackMontageNotifyWindow : public UAnimNotify_PlayMontageNotifyWindow
{
	GENERATED_BODY()

	//通知开始
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;

	//通知结束
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageAN")
	TObjectPtr<AMageCharacter> MageCharacter;

};
