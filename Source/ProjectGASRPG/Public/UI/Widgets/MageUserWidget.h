#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MageUserWidget.generated.h"

UCLASS()
class PROJECTGASRPG_API UMageUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(UObject* InWidgetController);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_UserWidget") 
	TObjectPtr<UObject> WidgetController;

protected:
	/* 蓝图中实现 */
	UFUNCTION(BlueprintImplementableEvent)
	void WidgetControllerSet();
};
