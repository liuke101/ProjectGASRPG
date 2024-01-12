#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class IInteractionInterface;
class AMageItem;

USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()
	FInteractionData() :
	CurrentInteractableActor(nullptr),
	LastInteractionCheckTime(0.f)
	{
	}
	
	UPROPERTY()
	AActor* CurrentInteractableActor;

	UPROPERTY()
	float LastInteractionCheckTime;
	
};

//增删改查委托
DECLARE_MULTICAST_DELEGATE_OneParam(FInventoryItemCRUD, const AMageItem* Item);

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

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

#pragma region Interaction
public:
	void PerformInteractionCheck();
	void FoundInteractable(AActor* NewInteractableActor);
	void NoInteractableFound();

	//按键绑定这三个函数，作为交互的接口
	void BeginInteract();
	void Interact();
	void EndInteract();
	
	FORCEINLINE bool IsInteracting() const { return GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_Interaction); }

	//实现接口的对象
	UPROPERTY(VisibleAnywhere,Category = "Inventory|Interaction")
	TScriptInterface<IInteractionInterface> TargetInteractableObject;

	UPROPERTY()
	FInteractionData InteractionData;

	UPROPERTY(EditAnywhere, Category = "Inventory|Interaction")
	float InteractionCheckFrequency = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Inventory|Interaction")
	float InteractionCheckDistance = 250.f;

	UPROPERTY()
	FTimerHandle TimerHandle_Interaction;
#pragma endregion

#pragma region CRUD
public:
	//增
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(AMageItem* Item);

	//删
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveItem(AMageItem* Item);

	//改
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapItem(AMageItem* ItemA, AMageItem* ItemB);

	//查
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	AMageItem* FindItemByTag(const FGameplayTag& Tag);

	/** 存储的Item */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<AMageItem*> Items;
#pragma endregion
};
