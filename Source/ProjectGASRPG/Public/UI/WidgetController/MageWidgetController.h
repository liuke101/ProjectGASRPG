#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MageWidgetController.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()
	
	FWidgetControllerParams(){}
	FWidgetControllerParams(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
		: PlayerController(PC)
		, PlayerState(PS)
		, AbilitySystemComponent(ASC)
		, AttributeSet(AS)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<APlayerController> PlayerController = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<APlayerState> PlayerState= nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent= nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<UAttributeSet> AttributeSet= nullptr;
};

UCLASS()
class PROJECTGASRPG_API UMageWidgetController : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetControllerParams(const FWidgetControllerParams& WCParams);
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<APlayerController> PlayerController;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<APlayerState> PlayerState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UAttributeSet> AttributeSet;
};
