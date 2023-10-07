// 


#include "UI/HUD/MageHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widgets/MageUserWidget.h"

void AMageHUD::BeginPlay()
{
	Super::BeginPlay();
	
	
	if (UUserWidget* UserWidget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass))
	{
		UserWidget->AddToViewport();
	}
}
