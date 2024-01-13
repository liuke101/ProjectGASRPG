// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

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

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTGASRPG_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

#pragma region Interaction
public:
	void PerformInteractionCheck();
	void FoundInteractableActor(AActor* NewInteractableActor);
	void NotFoundInteractableActor();

	//按键绑定这三个函数，作为交互的接口
	void BeginInteract();
	void Interact();
	void EndInteract();
	
	FORCEINLINE bool IsInteracting() const { return GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_Interaction); }

	//保存实现接口的对象, 这种方法比每次遇到交互对象都去Cast更方便
	UPROPERTY(VisibleAnywhere,Category = "Interaction")
	TScriptInterface<IInteractionInterface> TargetInteractableObject;

	UPROPERTY()
	FInteractionData InteractionData;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionCheckFrequency = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionCheckDistance = 250.f;

	UPROPERTY()
	FTimerHandle TimerHandle_Interaction;
#pragma endregion
};
