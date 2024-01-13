// 

#pragma once

#include "CoreMinimal.h"
#include "MageUserWidget.h"
#include "Inventory/Interface/InteractionInterface.h"
#include "InteractionWidget.generated.h"

class UProgressBar;
class UTextBlock;
/**
 * 
 */

/** 已弃用 */
UCLASS()
class PROJECTGASRPG_API UInteractionWidget : public UMageUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	
public:

protected:
	//meta = (BindWidget) 再控件蓝图中必须创建同名控件
	UPROPERTY(VisibleAnywhere, meta = (BindWidget),Category = "InteractionWidget")
	UTextBlock* NameText;
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget),Category = "InteractionWidget")
	UTextBlock* ActionText;
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget),Category = "InteractionWidget")
	UTextBlock* QuantityText;
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget),Category = "InteractionWidget")
	UTextBlock* KerPressText;
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget),Category = "InteractionWidget")
	UProgressBar* InteractionProgressBar;
	
	UPROPERTY(VisibleAnywhere, meta = (BindWidget),Category = "InteractionWidget")
	float CurrentInteractionDuration;

	UFUNCTION(Category = "InteractionWidget")
	float UpdateInteractionProgress();
};
