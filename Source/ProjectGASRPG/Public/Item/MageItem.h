// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractionInterface.h"
#include "MageItem.generated.h"

class USphereComponent;
class UWidgetComponent;
class UGameplayEffect;

USTRUCT()
struct FItemDetails
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemName = FName();
	
	UPROPERTY()
	FText ItemDescription = FText();
	
	UPROPERTY()
	TObjectPtr<UTexture2D> ItemIcon = nullptr;
	
	UPROPERTY()
	TObjectPtr<USkeletalMesh> ItemMesh = nullptr; //注意这里是UStaticMesh，而不是UStaticMeshComponent
	
	UPROPERTY()
	int32 ItemNum = 0;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> ItemWidget;
	
	UPROPERTY()
	TSubclassOf<UGameplayEffect> ItemGE = nullptr;

	UPROPERTY()
	FGameplayTag ItemTag = FGameplayTag::EmptyTag;
};

UCLASS()
class PROJECTGASRPG_API AMageItem : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	AMageItem();
	
protected:
	virtual void BeginPlay() override;
public:
	void SetItemDetails(const FItemDetails& NewItemDetails);
	FItemDetails GetItemDetails() const;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USphereComponent* SphereComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	FName ItemName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	FText ItemDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	TObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	TObjectPtr<USkeletalMeshComponent> ItemMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	int32 ItemNum;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	TObjectPtr<UWidgetComponent> ItemPickUpWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	TSubclassOf<UGameplayEffect> ItemGE;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MageItem|Details")
	FGameplayTag ItemTag;
};
