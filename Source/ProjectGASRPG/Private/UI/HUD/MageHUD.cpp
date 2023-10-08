// 


#include "UI/HUD/MageHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "UI/Widgets/MageUserWidget.h"

UOverlayWidgetController* AMageHUD::GetOverlayWidgetController(const FWidgetControllerParams WCParams)
{
	if(OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
	}
	return OverlayWidgetController;
}

void AMageHUD::InitOverlayWidget(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC,
	UAttributeSet* AS)
{
	checkf(OverlayWidgetClass, TEXT("OverlayWidgetClass 为空，请在 BP_MageHUD 蓝图中设置"));
	checkf(OverlayWidgetControllerClass	, TEXT("OverlayWidgetControllerClass 为空，请在 BP_MageHUD 蓝图中设置"));
	
	UUserWidget* UserWidget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget = Cast<UMageUserWidget>(UserWidget);
	OverlayWidget->SetWidgetController(GetOverlayWidgetController(FWidgetControllerParams(PC, PS, ASC, AS)));
	OverlayWidget->AddToViewport();
}


