// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AttributeInfo.generated.h"

USTRUCT(BlueprintType)
struct FMageAttributeInfo
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeName = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeDescription = FText();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float AttributeValue = 0.0f;
};
/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UAttributeInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	FMageAttributeInfo FindAttributeInfoForTag(const FGameplayTag& AttributeTag,bool blogNotFound = false) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FMageAttributeInfo> AttributeInfos;
	
};
