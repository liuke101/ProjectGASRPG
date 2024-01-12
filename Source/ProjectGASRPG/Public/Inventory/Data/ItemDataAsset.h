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

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MinPhysicalAttack;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MaxPhysicalAttack;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MinMagicAttack;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MaxMagicAttack;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float HealthRestorationAmount;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float ManaRestorationAmount;
};

/** 物品文本数据 */
USTRUCT(BlueprintType)
struct FItemTextData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;
};

/** 物品数值数据 */
USTRUCT(BlueprintType)
struct FItemNumericData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 ItemNum;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int MaxStackSize;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bIsStackable;
};

/** 物品资产数据 */
USTRUCT(BlueprintType)
struct FItemAssetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> ItemGE = nullptr;

	//头顶拾取信息
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSubclassOf<UMageUserWidget> ItemPickUpMessageWidget = nullptr;
};

USTRUCT(BlueprintType)
struct FMageItemInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ItemTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ItemInfo")
	EItemType ItemType;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ItemInfo")
	EItemQuality ItemQuality;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ItemInfo")
	FItemStatistics ItemStatistics;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ItemInfo")
	FItemTextData ItemTextData;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ItemInfo")
	FItemNumericData ItemNumericData;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="ItemInfo")
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
