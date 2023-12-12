#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTask_TargetingActorChanged.generated.h"


class AMagePlayerController;
struct FOnAttributeChangeData;


/**
 * （弃用）和内置的 AbilityTask：AbilityAsync_WaitAttributeChanged 功能重复，还是选择使用内置节点。
 * 蓝图节点，用于为 ASC 中的所有的 Attribute 更改自动注册监听器。
 * 可在控件蓝图中使用。
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class PROJECTGASRPG_API UAsyncTask_TargetingActorChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTargettingChanged, AActor*, NewTargetingActor, AActor*, OldTargetingActor);
	
	/** 委托作为节点的输出引脚 */
	UPROPERTY(BlueprintAssignable)
	FOnTargettingChanged OnTargetingChanged;
	
	/** 监听属性变化 */
	UFUNCTION(BlueprintCallable, Category = "MageAsyncTask|GAS", meta = (BlueprintInternalUseOnly = "true"))	
	static UAsyncTask_TargetingActorChanged* ListenForTargetingActorChanged(APlayerController* PlayerController);

	/**
	 * 要结束 AsyncTask 时，必须手动调用该函数。
	 * 对于 UMG Widget，您可以在 Widget 的 Destruct 事件中调用它。
	 */
	UFUNCTION(BlueprintCallable)
	void EndTask();

protected:
	UFUNCTION()
	void TargetingActorChangedCallback(AActor* NewTargetingActor, AActor* OldTargetingActor) const;
	
	UPROPERTY()
	APlayerController* PlayerController;
};
