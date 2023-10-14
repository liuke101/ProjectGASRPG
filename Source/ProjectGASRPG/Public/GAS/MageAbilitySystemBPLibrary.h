// 

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MageAbilitySystemBPLibrary.generated.h"

class UOverlayWidgetController;
/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UMageAbilitySystemBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Mage_AbilitySystemBPLibrary|WidgetController")
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);
};
