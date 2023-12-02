// 

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTaskAttributeChanged.generated.h"

struct FOnAttributeChangeData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttributeChanged, FGameplayAttribute, Attribute, float, NewValue,
                                               float, OldValue);

/**
 * 蓝图节点，用于为 ASC 中的所有的 Attribute 更改自动注册监听器。
 * 可在控件蓝图中使用。
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class PROJECTGASRPG_API UAsyncTaskAttributeChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** 委托作为节点的输出引脚 */
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChanged OnAttributeChanged;
	
	/** 监听属性变化 */
	UFUNCTION(BlueprintCallable, Category = "MageAsyncTasks|GAS", meta = (BlueprintInternalUseOnly = "true"))	
	static UAsyncTaskAttributeChanged* ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute);

	/**
	 * 监听属性变化
	 * 该版本接收一个 Attribute 数组。检查 Attribute 发生变化的 Attribute 输出。
	 */
	UFUNCTION(BlueprintCallable, Category = "MageAsyncTask|GAS", meta = (BlueprintInternalUseOnly = "true"))	
	static UAsyncTaskAttributeChanged* ListenForAttributesChange(UAbilitySystemComponent* AbilitySystemComponent, TArray<FGameplayAttribute> Attributes);

	/**
	 * 要结束 AsyncTask 时，必须手动调用该函数。
	 * 对于 UMG Widget，您可以在 Widget 的 Destruct 事件中调用它。
	 */
	UFUNCTION(BlueprintCallable)
	void EndTask();

protected:
	UPROPERTY()
	UAbilitySystemComponent* ASC;

	/** 监听的属性 */
	FGameplayAttribute Attribute;
	TArray<FGameplayAttribute> Attributes;

	void AttributeChangedCallback(const FOnAttributeChangeData& Data) const;
};
