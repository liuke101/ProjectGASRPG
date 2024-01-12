// 

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Data/ItemDataAsset.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTask_ItemPickedUp.generated.h"

class AMageItem;
class UMageWidgetController;

/**
 * AsyncTask节点: 监听物品拾取
 * - meta = (ExposedAsyncProxy = AsyncTask) 会输出名为AsyncTask的异步代理
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class PROJECTGASRPG_API UAsyncTask_ItemPickedUp : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** ItemDataAsset委托 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMageItemInfoDelegate, const AMageItem*, MageItem, const FMageItemInfo&, MageItemInfo);

	/** 委托作为节点的输出引脚 */
	UPROPERTY(BlueprintAssignable)
	FMageItemInfoDelegate OnItemPickedUp;

	/** 监听拾取物品 */
	UFUNCTION(BlueprintCallable, Category = "MageAsyncTask|GAS", meta = (BlueprintInternalUseOnly = "true"))	
	static UAsyncTask_ItemPickedUp* ListenForItemPickedUp(UMageWidgetController* MageWidgetController);

	/**
	 * 要结束 AsyncTask 时，必须手动调用该函数。
	 * 对于 UMG Widget，您可以在 Widget 的 Destruct 事件中调用它。
	 */
	UFUNCTION(BlueprintCallable)
	void EndTask();

protected:
	UFUNCTION()
	void ItemPickedUpCallback(const AMageItem* MageItem, const FMageItemInfo& MageItemInfo) const;
	
	UPROPERTY()
	TObjectPtr<UMageWidgetController> MageWidgetController;
};
