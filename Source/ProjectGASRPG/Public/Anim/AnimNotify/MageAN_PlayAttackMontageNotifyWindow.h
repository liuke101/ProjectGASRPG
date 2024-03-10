// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AnimGraphRuntime/Public/AnimNotifies/AnimNotify_PlayMontageNotify.h"
#include "MageAN_PlayAttackMontageNotifyWindow.generated.h"

class AMageCharacterBase;
/**
 * 
 */
UCLASS(editinlinenew, const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Play Attack Montage Notify Window"))
class PROJECTGASRPG_API UMageAN_PlayAttackMontageNotifyWindow : public UAnimNotify_PlayMontageNotifyWindow
{
	GENERATED_BODY()
	
	UMageAN_PlayAttackMontageNotifyWindow(const FObjectInitializer& ObjectInitializer);
	
	//通知开始
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;

	//通知结束
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;

	UPROPERTY()
	TObjectPtr<AMageCharacterBase> MageCharacter;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayEvent")
	FGameplayTag GameplayEventTag;
};
