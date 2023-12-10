// 

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor_Trace.h"
#include "AbilityTargetActor_CursorTrace.generated.h"

UCLASS()
class PROJECTGASRPG_API AAbilityTargetActor_CursorTrace : public AGameplayAbilityTargetActor_Trace
{
	GENERATED_BODY()
public:
	/*  开始瞄准时调用，只会调用一次 */ 
	virtual void StartTargeting(UGameplayAbility* Ability) override;

	/* 蓝图事件，可以根据检测结果，显示不同的效果*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting")
	void SetStyleOfVisiableComponent(bool bWasRightPosition); 
protected:
	/* 重写PerformTrace方法*/ 
	virtual FHitResult PerformTrace(AActor* InSourceActor) override;

	/* 确认方法，若返回false，即便是进行Confirm操作也会被阻塞*/ 
	virtual bool IsConfirmTargetingAllowed() override;

	bool bLastTraceWasGood;
	
};
