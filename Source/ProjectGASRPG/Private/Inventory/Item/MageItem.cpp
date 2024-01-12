#include "Inventory/Item/MageItem.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
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

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
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


