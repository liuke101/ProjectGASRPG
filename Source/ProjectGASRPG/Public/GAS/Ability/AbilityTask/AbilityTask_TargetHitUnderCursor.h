#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_TargetHitUnderCursor.generated.h"

/** 节点的多个输出引脚都是由委托实现的 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMouseTargetDataSignature, const FGameplayAbilityTargetDataHandle&, TargetDataHandle);

/** 发送鼠标指针TargetData */
UCLASS()
class PROJECTGASRPG_API UAbilityTask_TargetHitUnderCursor : public UAbilityTask
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_TargetHitUnderCursor* WaitTargetHitUnderCursor(UGameplayAbility* OwningAbility);

	UPROPERTY(BlueprintAssignable)
	FMouseTargetDataSignature ValidData;

	virtual void Activate() override;

protected:
	void SendCursorHitData();

};
