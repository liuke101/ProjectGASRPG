#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTask_InventoryItemChanged.generated.h"

struct FMageItemInfo;
class AMageItem;
class UInventoryComponent;


UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class PROJECTGASRPG_API UAsyncTask_InventoryItemChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	/** Item增删改查委托 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChanged, const AMageItem*, Item);

	/** 委托作为节点的输出引脚 */
	UPROPERTY(BlueprintAssignable)
	FInventoryItemChanged OnItemAdded;
	
	UPROPERTY(BlueprintAssignable)
	FInventoryItemChanged OnItemRemoved;

	/** 监听物品增删改查 */
	UFUNCTION(BlueprintCallable, Category = "MageAsyncTask|Inventory", meta = (BlueprintInternalUseOnly = "true"))	
	static UAsyncTask_InventoryItemChanged* ListenForInventoryItemChanged(UInventoryComponent* InventoryComponent);

	/**
	 * 要结束 AsyncTask 时，必须手动调用该函数。
	 * 对于 UMG Widget，您可以在 Widget 的 Destruct 事件中调用它。
	 */
	UFUNCTION(BlueprintCallable)
	void EndTask();

protected:
	UFUNCTION()
	void ItemAddedCallback(const AMageItem* Item) const;
	UFUNCTION()
	void ItemRemovedCallback(const AMageItem* Item) const;
	
	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryComponent;
};
