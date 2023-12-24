// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "GAS/Data/ItemDataAsset.h"
#include "Interface/InteractionInterface.h"
#include "MageItem.generated.h"

class UItemDataAsset;
class USphereComponent;
class UWidgetComponent;
class UGameplayEffect;

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

	UFUNCTION(BlueprintCallable, Category = "MageItem|Info")
	FORCEINLINE FGameplayTag GetItemTag() const { return ItemTag; }
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

#pragma region InteractionInterface
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
#pragma endregion
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* SphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;
	
	//拾取提示
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<UWidgetComponent> PickUpTipsWidget;
	
	/** 数据资产 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UItemDataAsset> ItemDataAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	FGameplayTag ItemTag;

	/** 物品信息  */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	FName ItemName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	FText ItemDescription;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	int32 ItemNum;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TSubclassOf<UGameplayEffect> ItemGE;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MageItem|Info")
	TSubclassOf<UMageUserWidget> PickUpMessageWidget;
};
