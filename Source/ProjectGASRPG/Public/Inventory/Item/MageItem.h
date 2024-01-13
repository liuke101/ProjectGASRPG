#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Inventory/Data/ItemDataAsset.h"
#include "Inventory/Interface/InteractionInterface.h"
#include "MageItem.generated.h"

class UInventoryComponent;
class AMageCharacter;
class UItemDataAsset;
class USphereComponent;
class UWidgetComponent;
class UGameplayEffect;

//TODO：标记可堆叠物品，可堆叠的物品可以叠加，不可堆叠的物品不可以叠加
UCLASS()
class PROJECTGASRPG_API AMageItem : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	AMageItem();
	
protected:
	virtual void BeginPlay() override;

	
#if WITH_EDITOR
	/** 编辑器状态中，属性改变时立刻更新。例如更换ItemTag，立刻更新DataAsset中匹配的属性 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
public:
	/** 初始化物品 */
	virtual void InitMageItem(int32 InQuantity);

	UFUNCTION(BlueprintCallable, Category = "MageItem|Info")
	FMageItemInfo GetDefaultMageItemInfo() const;

	UFUNCTION(BlueprintCallable, Category = "MageItem|Info")
	void SetMageItemInfo(const FMageItemInfo& InMageItemInfo);

public:
	UFUNCTION(BlueprintCallable, Category = "MageItem|Info")
	AMageItem* CreateItemCopy() const;
	
	UFUNCTION(Category = "MageItem|Info")
	FORCEINLINE bool IsFullItemStack() const { return Quantity == ItemNumericData.MaxStackSize; }
	
	UFUNCTION(Category = "MageItem|Info")
	void SetQuantity(const int32 NewQuantity);
	
	UFUNCTION(Category = "MageItem|Info")
	virtual void Use(AMageCharacter* MageCharacter);

	/** Item是否在背包中 */
	bool bIsInInventory = false;
public:
	/** 数据资产 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem")
	TObjectPtr<UItemDataAsset> ItemDataAsset;

	/** 物品标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem")
	FGameplayTag ItemTag;
	
	/** 物品数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem")
	int32 Quantity;
	
	/** 物品信息  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageItem|Info")
	EItemType ItemType;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageItem|Info")
	EItemQuality ItemQuality;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageItem|Info")
	FItemStatistics ItemStatistics;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category="MageItem|Info")
	FItemTextData ItemTextData;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageItem|Info")
	FItemNumericData ItemNumericData;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category="MageItem|Info")
	FItemAssetData ItemAssetData;

#pragma region InteractionInterface
	virtual void BeginFocus() override;
	virtual void EndFocus() override;
	
	virtual void BeginInteract() override; //按下F触发
	virtual void Interact() override; //按下F后持续触发
	virtual void EndInteract() override; //松开F触发
	
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
	
	virtual void UpdateInteractableData() override;
	FORCEINLINE virtual FInteractableData GetInteractableData() override {return InteractableData;}
	//由UpdateInteractableData()设置
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageItem|InteractableData")
	FInteractableData InteractableData;
#pragma endregion

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	//交互按键提示
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<UWidgetComponent> InteractKeyWidget;
};
