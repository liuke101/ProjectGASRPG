#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MageWidgetController.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;

UCLASS()
class PROJECTGASRPG_API UMageWidgetController : public UObject
{
	GENERATED_BODY()

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
