#include "Inventory/Item/MageItem.h"

#include "LocalizationDescriptor.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Inventory/Component/InventoryComponent.h"
#include "Inventory/Data/ItemDataAsset.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

AMageItem::AMageItem()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(SphereComponent);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InteractKeyWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	InteractKeyWidget->SetupAttachment(RootComponent);
	InteractKeyWidget->SetVisibility(false);
	InteractKeyWidget->SetWidgetSpace(EWidgetSpace::Screen);
	static ConstructorHelpers::FClassFinder<UUserWidget> InteractKeyWidgetClass(TEXT("/Game/Blueprints/UI/UserWidget/Overlay/Interact/WBP_InteractKeyWidget"));
	if(InteractKeyWidgetClass.Succeeded())
	{
		InteractKeyWidget->SetWidgetClass(InteractKeyWidgetClass.Class);
	}
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetRenderCustomDepth(true);
	MeshComponent->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
}

void AMageItem::BeginPlay()
{
	Super::BeginPlay();
	
	InitMageItem(Quantity);
}

void AMageItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if(ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AMageItem, ItemTag))
	{
		InitMageItem(Quantity); //重新初始化
	}
}

void AMageItem::InitMageItem(int32 InQuantity)
{
	SetMageItemInfo(GetDefaultMageItemInfo());
	InQuantity <= 0 ? Quantity = 1 : Quantity = InQuantity;
	MeshComponent->SetStaticMesh(ItemAssetData.ItemMesh);
	UpdateInteractableData();
}

FMageItemInfo AMageItem::GetDefaultMageItemInfo() const
{
	if(ItemDataAsset && ItemTag.IsValid())
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

	ItemCopy->ItemDataAsset = this->ItemDataAsset;
	ItemCopy->ItemTag = this->ItemTag;
	ItemCopy->Quantity = this->Quantity;
	
	ItemCopy->ItemType = this->ItemType;
	ItemCopy->ItemQuality = this->ItemQuality;
	ItemCopy->ItemStatistics = this->ItemStatistics;
	ItemCopy->ItemTextData = this->ItemTextData;
	ItemCopy->ItemNumericData = this->ItemNumericData;
	ItemCopy->ItemAssetData = this->ItemAssetData;
	
	return ItemCopy;
}

void AMageItem::SetQuantity(const int32 NewQuantity)
{
	if(NewQuantity != Quantity)
	{
		Quantity = FMath::Clamp(NewQuantity, 0, ItemNumericData.bIsStackable ? ItemNumericData.MaxStackSize : 1);
	}
}

void AMageItem::Use(AMageCharacter* MageCharacter)
{
	
}

void AMageItem::BeginFocus()
{
	HighlightActor();
	InteractKeyWidget->SetVisibility(true);
}

void AMageItem::EndFocus()
{
	UnHighlightActor();
	InteractKeyWidget->SetVisibility(false);
}

void AMageItem::BeginInteract()
{
}

void AMageItem::Interact()
{
}

void AMageItem::EndInteract()
{
}

void AMageItem::UpdateInteractableData()
{
	InteractableData.InteractableType = EInteractableType::Pickup;
	InteractableData.Name = ItemTextData.Name;
	InteractableData.Action = ItemTextData.ActionDescription;
	InteractableData.Quantity = Quantity;
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


