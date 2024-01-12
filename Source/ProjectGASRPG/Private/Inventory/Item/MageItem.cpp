#include "Inventory/Item/MageItem.h"

#include "LocalizationDescriptor.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Inventory/Component/InventoryComponent.h"
#include "Inventory/Data/ItemDataAsset.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

AMageItem::AMageItem()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);

	PickUpTipsWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	PickUpTipsWidget->SetupAttachment(RootComponent);
	PickUpTipsWidget->SetVisibility(false);
	PickUpTipsWidget->SetWidgetSpace(EWidgetSpace::Screen);
	static ConstructorHelpers::FClassFinder<UUserWidget> PickUpTipsWidgetClass(TEXT("/Game/Blueprints/UI/UserWidget/Overlay/Interact/WBP_PickUpTipsWidget"));
	if(PickUpTipsWidgetClass.Succeeded())
	{
		PickUpTipsWidget->SetWidgetClass(PickUpTipsWidgetClass.Class);
	}

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRenderCustomDepth(true);
	MeshComponent->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
}

void AMageItem::BeginPlay()
{
	Super::BeginPlay();
	
	InitMageItemInfo();
	InteractableData = InstanceInteractableData;
}

void AMageItem::InitMageItemInfo()
{
	SetMageItemInfo(GetDefaultMageItemInfo());
}

FMageItemInfo AMageItem::GetDefaultMageItemInfo() const
{
	if(ItemDataAsset)
	{
		const FMageItemInfo MageItemInfo = ItemDataAsset->FindMageItemInfoForTag(ItemTag, true);
		return MageItemInfo;
	}
	return FMageItemInfo();
}

void AMageItem::SetMageItemInfo(const FMageItemInfo& InMageItemInfo)
{
	ItemType = InMageItemInfo.ItemType;
	ItemQuality = InMageItemInfo.ItemQuality;
	ItemStatistics = InMageItemInfo.ItemStatistics;
	ItemTextData = InMageItemInfo.ItemTextData;
	ItemNumericData = InMageItemInfo.ItemNumericData;
	ItemAssetData = InMageItemInfo.ItemAssetData;
}

AMageItem* AMageItem::CreateItemCopy() const
{
	AMageItem* ItemCopy = NewObject<AMageItem>(StaticClass());
	
	ItemCopy->ItemTag = this->ItemTag;
	ItemCopy->Quantity = this->Quantity;
	ItemCopy->ItemType = this->ItemType;
	ItemCopy->ItemQuality = this->ItemQuality;
	ItemCopy->ItemStatistics = this->ItemStatistics;
	ItemCopy->ItemTextData = this->ItemTextData;
	ItemCopy->ItemNumericData = this->ItemNumericData;
	ItemCopy->ItemAssetData = this->ItemAssetData;
	
	ItemCopy->ItemDataAsset = this->ItemDataAsset;
	
	return ItemCopy;
}

void AMageItem::SetQuantity(const int32 NewQuantity)
{
	if(NewQuantity != Quantity)
	{
		Quantity = FMath::Clamp(NewQuantity, 0, ItemNumericData.bIsStackable ? ItemNumericData.MaxStackSize : 1);

		// if(InventoryComponent)
		// {
		// 	if(Quantity<=0)
		// 	{
		// 		InventoryComponent->RemoveItem(this);
		// 	}
		// }
	}
}

void AMageItem::Use(AMageCharacter* MageCharacter)
{
	
}

void AMageItem::BeginFocus()
{
	HighlightActor();
	PickUpTipsWidget->SetVisibility(true);
}

void AMageItem::EndFocus()
{
	UnHighlightActor();
	PickUpTipsWidget->SetVisibility(false);
}

void AMageItem::BeginInteract()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("BeginInteract"));
}

void AMageItem::Interact(UInventoryComponent* OwnerInventoryComponent)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("Interact"));
}

void AMageItem::EndInteract()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("EndInteract"));
}


void AMageItem::HighlightActor()
{
	if(MeshComponent)
	{
		MeshComponent->SetCustomDepthStencilValue(HighlightItemStencilMaskValue);
	}
}

void AMageItem::UnHighlightActor()
{
	if(MeshComponent)
	{
		MeshComponent->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
	}
}


