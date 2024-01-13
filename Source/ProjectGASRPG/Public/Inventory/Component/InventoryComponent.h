#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class AMageItem;

//增删改查委托
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryItemCRUD, const AMageItem* Item);

/** 库存添加结果 */	
UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NotItemAdded UMETA(DisplayName = "NotItemAdded"),         //不添加
	IAR_PartialItemAdded UMETA(DisplayName = "PartialItemAdded"), //部分添加
	IAR_AllItemAdded UMETA(DisplayName = "FullItemAdded")		  //全部添加
};

USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()
	FItemAddResult() :
	ActualAmountAdded(0),
	Result(EItemAddResult::IAR_NotItemAdded),
	ResultMessage(FText::GetEmpty())
	{
	}

	/** 库存实际添加的数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|FItemAddResult")
	int32 ActualAmountAdded;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|FItemAddResult")
	EItemAddResult Result;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|FItemAddResult")
	FText ResultMessage;

	static FItemAddResult AddedNone(const FText& ErrorText)
	{
		FItemAddResult ItemAddResult;
		ItemAddResult.ActualAmountAdded = 0;
		ItemAddResult.Result = EItemAddResult::IAR_NotItemAdded;
		ItemAddResult.ResultMessage = ErrorText;
		return ItemAddResult;
	}
	static FItemAddResult AddedPartial(const FText& ErrorText, const int32& AddedAmount)
	{
		FItemAddResult ItemAddResult;
		ItemAddResult.ActualAmountAdded = AddedAmount;
		ItemAddResult.Result = EItemAddResult::IAR_PartialItemAdded;
		ItemAddResult.ResultMessage = ErrorText;
		return ItemAddResult;
	}
	static FItemAddResult AddedAll(const FText& ErrorText, const int32& AddedAmount)
	{
		FItemAddResult ItemAddResult;
		ItemAddResult.ActualAmountAdded = AddedAmount;
		ItemAddResult.Result = EItemAddResult::IAR_AllItemAdded;
		ItemAddResult.ResultMessage = ErrorText;
		return ItemAddResult;
	}
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	FInventoryItemCRUD OnItemAdded;
	FInventoryItemCRUD OnItemRemoved;
protected:
	virtual void BeginPlay() override;

#pragma region CRUD
public:
	//增
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult HandleAddItem(AMageItem* InItem);

	//删
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveSingleInstanceOfItem(AMageItem* ItemToRemove);
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveAmountOfItem(AMageItem* InItem, int32 RemoveAmount);
	
	//改
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SplitItemStack(AMageItem* InItem, int32 SplitAmount);

	//查
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AMageItem* FindMatchingItem(AMageItem* InItem);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AMageItem* FindItemByTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AMageItem* FindNextItemByTag(AMageItem* InItem);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AMageItem* FindNextPartialStack(AMageItem* InItem);

	//Getter/Setter
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FORCEINLINE void SetSlotsCapacity(const int32 NewCapacity){ InventorySlotsCapacity = NewCapacity; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FORCEINLINE int32 GetSlotsCapacity() const { return InventorySlotsCapacity; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FORCEINLINE TArray<AMageItem*> GetInventoryContents() const { return InventoryContents; }

protected:
	/** 存储的Items */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<TObjectPtr<AMageItem>> InventoryContents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventorySlotsCapacity = 99;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult HandleNonStackableItems(AMageItem* InItem,int32 RequestedAddAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 HandleStackableItems(AMageItem* InItem,int32 RequestedAddAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 CalcNumberForFullStack(AMageItem* StackableItem,int32 InitialRequestedAddAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddNewItem(AMageItem* InItem, const int32 AddAmount);
	
#pragma endregion
};
