// 


#include "UI/HUD/MageHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "UI/WidgetController/SkillTreeWidgetController.h"
#include "UI/Widgets/MageUserWidget.h"

UOverlayWidgetController* AMageHUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	if(OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		
		OverlayWidgetController->BindCallbacks(); 
	}
	return OverlayWidgetController;
}

UAttributeMenuWidgetController* AMageHUD::GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams)
{
	if(AttributeMenuWidgetController == nullptr)
	{
		AttributeMenuWidgetController = NewObject<UAttributeMenuWidgetController>(this, AttributeMenuWidgetControllerClass);
		AttributeMenuWidgetController->SetWidgetControllerParams(WCParams);
		
		AttributeMenuWidgetController->BindCallbacks(); 
	}
	return AttributeMenuWidgetController;
}

USkillTreeWidgetController* AMageHUD::GetSkillTreeWidgetController(const FWidgetControllerParams& WCParams)
{
	if(SkillTreeWidgetController == nullptr)
	{
		SkillTreeWidgetController = NewObject<USkillTreeWidgetController>(this, SkillTreeWidgetControllerClass);
		SkillTreeWidgetController->SetWidgetControllerParams(WCParams);
		
		SkillTreeWidgetController->BindCallbacks(); 
	}
	return SkillTreeWidgetController;
}

void AMageHUD::InitOverlayWidget(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC,
                                 UAttributeSet* AS)
{
	checkf(OverlayWidgetClass, TEXT("OverlayWidgetClass 为空，请在 BP_MageHUD 蓝图中设置"));
	checkf(OverlayWidgetControllerClass	, TEXT("OverlayWidgetControllerClass 为空，请在 BP_MageHUD 蓝图中设置"));

	// 创建 OverlayWidget
	UUserWidget* UserWidget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget = Cast<UMageUserWidget>(UserWidget);

	if(OverlayWidget)
	{
		// 获取 OverlayWidgetController
		UOverlayWidgetController* WidgetController = GetOverlayWidgetController(FWidgetControllerParams(PC, PS, ASC, AS));
		
		// 设置 OverlayWidget 的 WidgetController
		OverlayWidget->SetWidgetController(WidgetController);
		
		// WidgetController 广播初始值，委托回调已经在上一行 SetWidgetController() 中中绑定
		WidgetController->BroadcastInitialValue();

		// 将 OverlayWidget 添加到 Viewport
		OverlayWidget->AddToViewport();
	}		
}


