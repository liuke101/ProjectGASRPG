#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ItemDataAsset.generated.h"

class UMageUserWidget;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FMageItemInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ItemTag = FGameplayTag::EmptyTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ItemName = FName();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ItemDescription = FText();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> ItemIcon = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ItemNum = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> ItemGE = nullptr;

	//头顶拾取信息
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UMageUserWidget> ItemPickUpMessageWidget = nullptr;
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
