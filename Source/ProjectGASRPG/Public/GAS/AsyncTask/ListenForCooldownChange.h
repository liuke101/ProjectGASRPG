#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ListenForCooldownChange.generated.h"

struct FActiveGameplayEffectHandle;
struct FGameplayEffectSpec;
class UAbilitySystemComponent;

/** 节点的多个输出引脚都是由委托实现的 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCooldownChangeSignature, float, TimeRemaining);

/**
 * AsyncTask节点: 监听冷却时间变化
 * - meta = (ExposedAsyncProxy = AsyncTask) 会输出名为AsyncTask的异步代理
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class PROJECTGASRPG_API UListenForCooldownChange : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** 委托作为节点的输出引脚 */
	UPROPERTY(BlueprintAssignable)
	FCooldownChangeSignature CooldownStart;

	UPROPERTY(BlueprintAssignable)
	FCooldownChangeSignature CooldownEnd;

	
	UFUNCTION(BlueprintCallable, Category = "Mage_AsyncTasks", meta = (BlueprintInternalUseOnly = "TRUE"))	
	static UListenForCooldownChange* ListenForCooldownChange(UAbilitySystemComponent* InASC, const FGameplayTag& InCooldownTag);

	UFUNCTION(BlueprintCallable)
	void EndTask();
	
	
protected:
	/** 节点的输入引脚 */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	FGameplayTag CooldownTag;

	void CooldownTagChangedCallback(const FGameplayTag GameplayTag, int32 NewCount) const;

	void OnActiveEffectAddedToSelfCallback(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle EffectHandle) const;
	
};


