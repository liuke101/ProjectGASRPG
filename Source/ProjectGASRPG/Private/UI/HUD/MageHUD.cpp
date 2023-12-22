// 


#include "UI/HUD/MageHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/EquipmentWidgetController.h"
#include "UI/WidgetController/InventoryWidgetController.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "UI/WidgetController/SkillTreeWidgetController.h"
#include "UI/Widgets/MageUserWidget.h"
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
		// 创建 OverlayWidgetController, 绑定属性值变化委托（属性值变化就广播属性值）
		OverlayWidgetController = GetOverlayWidgetController(FWidgetControllerParams(PC, PS, ASC, AS));
		
		// 设置 OverlayWidgetController, 绑定UI的委托回调
		OverlayWidget->SetWidgetController(OverlayWidgetController);
		
		// OverlayWidgetController 广播属性初始值，初始化UI
		OverlayWidgetController->BroadcastInitialValue();

		// 将 OverlayWidget 添加到 Viewport
		OverlayWidget->AddToViewport();
	}
}

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

UEquipmentWidgetController* AMageHUD::GetEquipmentWidgetController(const FWidgetControllerParams& WCParams)
{
	if(EquipmentWidgetController == nullptr)
	{
		EquipmentWidgetController = NewObject<UEquipmentWidgetController>(this, EquipmentWidgetControllerClass);
		EquipmentWidgetController->SetWidgetControllerParams(WCParams);
		
		EquipmentWidgetController->BindCallbacks(); 
	}
	return EquipmentWidgetController;
}

UInventoryWidgetController* AMageHUD::GetInventoryWidgetController(const FWidgetControllerParams& WCParams)
{
	if(InventoryWidgetController == nullptr)
	{
		InventoryWidgetController = NewObject<UInventoryWidgetController>(this, InventoryWidgetControllerClass);
		InventoryWidgetController->SetWidgetControllerParams(WCParams);
		
		InventoryWidgetController->BindCallbacks(); 
	}

	return InventoryWidgetController;
}

