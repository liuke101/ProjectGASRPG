#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "TargetDataUnderMouse.generated.h"

/** 节点的多个输出引脚都是由委托实现的 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMouseTargetDataSignature, const FGameplayAbilityTargetDataHandle&, TargetDataHandle);

/** 发送鼠标指针TargetData */
UCLASS()
class PROJECTGASRPG_API UTargetDataUnderMouse : public UAbilityTask
{
	GENERATED_BODY()

public:
	/** latent 蓝图节点 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",meta = (DisplayName = "TargetDataUnderMouse", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UTargetDataUnderMouse* CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility);

	UPROPERTY(BlueprintAssignable)
	FMouseTargetDataSignature ValidData;

private:
	virtual void Activate() override;

	/** 客户端复制鼠标指针的 TargetData 到服务器 */
	void SendMouseCursorData();

	void OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ActivationTag);
};
