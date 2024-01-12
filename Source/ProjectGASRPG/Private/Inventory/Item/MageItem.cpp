#include "Inventory/Item/MageItem.h"
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

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRenderCustomDepth(true);
	MeshComponent->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
}

void AMageItem::BeginPlay()
{
	Super::BeginPlay();
	
	InitMageItemInfo();
	
	// 绑定碰撞委托
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AMageItem::OnSphereBeginOverlap);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &AMageItem::OnSphereEndOverlap);
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
	// ItemName = InMageItemInfo.ItemName;
	// ItemDescription = InMageItemInfo.ItemDescription;
	// ItemIcon = InMageItemInfo.ItemIcon;
	// ItemNum = InMageItemInfo.ItemNum;
	// ItemGE = InMageItemInfo.ItemGE;
	// PickUpMessageWidget = InMageItemInfo.ItemPickUpMessageWidget;
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

void AMageItem::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	PickUpTipsWidget->SetVisibility(true);
	HighlightActor();
}

void AMageItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	PickUpTipsWidget->SetVisibility(false);
	UnHighlightActor();
}

void AMageItem::BeginFocus()
{
	HighlightActor();
}

void AMageItem::EndFocus()
{
	UnHighlightActor();
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


