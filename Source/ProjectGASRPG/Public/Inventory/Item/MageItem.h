﻿#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Inventory/Data/ItemDataAsset.h"
#include "Interface/InteractionInterface.h"
#include "MageItem.generated.h"

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
	
public:
	void InitMageItemInfo();

	UFUNCTION(BlueprintCallable, Category = "MageItem|Info")
	FMageItemInfo GetDefaultMageItemInfo() const;

	UFUNCTION(BlueprintCallable, Category = "MageItem|Info")
	void SetMageItemInfo(const FMageItemInfo& InMageItemInfo);
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

#pragma region InteractionInterface
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
#pragma endregion
public:
	UFUNCTION(Category = "MageItem|Info")
	FORCEINLINE bool IsFullItemStack() const { return Quantity == ItemNumericData.MaxStackSize; }
	
	UFUNCTION(Category = "MageItem|Info")
	void SetQuantity(const int32 NewQuantity);
	
	UFUNCTION(Category = "MageItem|Info")
	virtual void Use(AMageCharacter* MageCharacter);
	
	/** 物品数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MageItem|Info")
	int32 Quantity;
	
	/** 物品信息  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	FGameplayTag ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageItem|Info")
	EItemType ItemType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageItem|Info")
	EItemQuality ItemQuality;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageItem|Info")
	FItemStatistics ItemStatistics;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="MageItem|Info")
	FItemTextData ItemTextData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageItem|Info")
	FItemNumericData ItemNumericData;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="MageItem|Info")
	FItemAssetData ItemAssetData;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* SphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;
	
	//拾取按键提示
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<UWidgetComponent> PickUpTipsWidget;

	/** 数据资产 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UItemDataAsset> ItemDataAsset;

	
};