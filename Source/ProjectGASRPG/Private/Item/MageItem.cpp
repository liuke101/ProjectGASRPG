#include "Item/MageItem.h"

#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

AMageItem::AMageItem()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);
	
	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	ItemMesh->SetupAttachment(RootComponent);
	ItemMesh->SetRenderCustomDepth(true);
	ItemMesh->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
	
	ItemPickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	ItemPickUpWidget->SetupAttachment(RootComponent);
	ItemPickUpWidget->SetVisibility(false);
}

void AMageItem::BeginPlay()
{
	Super::BeginPlay();

	// 绑定碰撞委托
	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AMageItem::OnSphereBeginOverlap);
	SphereComponent->OnComponentEndOverlap.AddDynamic(this, &AMageItem::OnSphereEndOverlap);
}

void AMageItem::SetItemDetails(const FItemDetails& NewItemDetails)
{
	ItemName = NewItemDetails.ItemName;
	ItemDescription = NewItemDetails.ItemDescription;
	ItemIcon = NewItemDetails.ItemIcon;
	ItemMesh->SetSkeletalMesh(NewItemDetails.ItemMesh);
	ItemNum = NewItemDetails.ItemNum;
	ItemPickUpWidget = NewItemDetails.ItemWidget;
	ItemGE = NewItemDetails.ItemGE;
	ItemTag = NewItemDetails.ItemTag;
}

FItemDetails AMageItem::GetItemDetails() const
{
	FItemDetails ItemDetails;
	ItemDetails.ItemName = ItemName;
	ItemDetails.ItemDescription = ItemDescription;
	ItemDetails.ItemIcon = ItemIcon;
	ItemDetails.ItemMesh = ItemMesh->GetSkeletalMeshAsset();
	ItemDetails.ItemNum = ItemNum;
	ItemDetails.ItemWidget = ItemPickUpWidget;
	ItemDetails.ItemGE = ItemGE;
	ItemDetails.ItemTag = ItemTag;
	return ItemDetails;
}

void AMageItem::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ItemPickUpWidget->SetVisibility(true);
	HighlightActor();
}

void AMageItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ItemPickUpWidget->SetVisibility(false);
	UnHighlightActor();
}

void AMageItem::HighlightActor()
{
	if(ItemMesh)
	{
		ItemMesh->SetCustomDepthStencilValue(HighlightActorStencilMaskValue);
	}
}

void AMageItem::UnHighlightActor()
{
	if(ItemMesh)
	{
		ItemMesh->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
	}
}


