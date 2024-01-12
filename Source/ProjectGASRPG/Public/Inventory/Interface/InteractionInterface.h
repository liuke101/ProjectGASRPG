// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

class UInventoryComponent;

UENUM()
enum class EInteractableType : uint8
{
	Pickup,
	Device, //门、窗户等
	NPC,    //NonePlayerCharacter
	Toggle,
	Container,
	
};

USTRUCT()
struct FInteractableData
{
	GENERATED_BODY()

	FInteractableData() :
	InteractableType(EInteractableType::Pickup),
	Name(FText::GetEmpty()),
	Action(FText::GetEmpty()),
	Quantity(0),
	InteractionDuration(0.f)
	{
	};

	UPROPERTY(EditInstanceOnly)
	EInteractableType InteractableType;
	
	UPROPERTY(EditInstanceOnly)
	FText Name;
	
	UPROPERTY(EditInstanceOnly)
	FText Action;

	//仅用于Pickup类型
	UPROPERTY(EditInstanceOnly)
	int32 Quantity;

	//用于需要持续交互的类型
	UPROPERTY(EditInstanceOnly)
	float InteractionDuration;
};

UINTERFACE()
class UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTGASRPG_API IInteractionInterface
{
	GENERATED_BODY()

public:
	virtual void BeginFocus();
	virtual void EndFocus();
	virtual void BeginInteract();
	virtual void Interact(UInventoryComponent *InventoryComponent);
	virtual void EndInteract();
	

	FInteractableData InteractableData;
	
	virtual void HighlightActor() = 0;
	virtual void UnHighlightActor() = 0;
};
