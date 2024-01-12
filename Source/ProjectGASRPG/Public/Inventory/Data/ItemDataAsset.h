#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

class UMageUserWidget;
class UGameplayEffect;

/** 物品品质(用颜色表示) */
UENUM()
enum class EItemQuality : uint8
{
	White,
	Green,
	Blue,
	Purple,
	Red
};

/** 物品类型 */
UENUM()
enum class EItemType : uint8
{
	Weapon,
	Consumables,
	Materials,
};

/** 物品统计数据 */
USTRUCT(BlueprintType)
struct FItemStatistics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float MinPhysicalAttack;
	
	UPROPERTY(EditAnywhere)
	float MaxPhysicalAttack;
	
	UPROPERTY(EditAnywhere)
	float MinMagicAttack;
	
	UPROPERTY(EditAnywhere)
	float MaxMagicAttack;
	
	UPROPERTY(EditAnywhere)
	float HealthRestorationAmount;
	
	UPROPERTY(EditAnywhere)
	float ManaRestorationAmount;
};

/** 物品文本数据 */
USTRUCT(BlueprintType)
struct FItemTextData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText Name;
	
	UPROPERTY(EditAnywhere)
	FText Description;
};

/** 物品数值数据 */
USTRUCT(BlueprintType)
struct FItemNumericData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 ItemNum;

	UPROPERTY(EditAnywhere)
	int MaxStackSize;
	
	UPROPERTY(EditAnywhere)
	bool bIsStackable;
};

/** 物品资产数据 */
USTRUCT(BlueprintType)
struct FItemAssetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> ItemGE = nullptr;

	//头顶拾取信息
	UPROPERTY(EditAnywhere)
	TSubclassOf<UMageUserWidget> ItemPickUpMessageWidget = nullptr;
};

USTRUCT(BlueprintType)
struct FMageItemInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ItemTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere,Category="ItemInfo")
	EItemType ItemType;
	UPROPERTY(EditAnywhere,Category="ItemInfo")
	EItemQuality ItemQuality;
	UPROPERTY(EditAnywhere,Category="ItemInfo")
	FItemStatistics ItemStatistics;
	UPROPERTY(EditAnywhere,Category="ItemInfo")
	FItemTextData ItemTextData;
	UPROPERTY(EditAnywhere,Category="ItemInfo")
	FItemNumericData ItemNumericData;
	UPROPERTY(EditAnywhere,Category="ItemInfo")
	FItemAssetData ItemAssetData;
};

UCLASS()
class PROJECTGASRPG_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FMageItemInfo> MageItemInfos;

	FMageItemInfo FindMageItemInfoForTag(const FGameplayTag& ItemTag,bool blogNotFound = false) const;
};
