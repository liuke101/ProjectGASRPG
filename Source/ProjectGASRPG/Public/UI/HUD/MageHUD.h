#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MageHUD.generated.h"

class UInventoryWidgetController;
class UEquipmentWidgetController;
class USkillTreeWidgetController;
class UAttributeMenuWidgetController;
class UAbilitySystemComponent;
class UAttributeSet;
class UOverlayWidgetController;
struct FWidgetControllerParams;
class UMageUserWidget;

UCLASS(BlueprintType, Blueprintable)
class PROJECTGASRPG_API AMageHUD : public AHUD
{
	GENERATED_BODY()

public:
	/*
	 * 初始化 OverlayWidget
	 * 在 MageCharacter 类 InitASC() 中调用
	 */
	void InitOverlayWidget(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

	/**
	 * 通过在HUD单例中创建WidgetController
	 * - 这样的好处时减少了全局变量类的数目，我们不需要令WidgetController为全局变量也可以通过HUD全局访问到WidgetController单例
	 * - 全局访问函数：UMageAbilitySystemLibrary::GetOverlayWidgetController
	 */
	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	USkillTreeWidgetController* GetSkillTreeWidgetController(const FWidgetControllerParams& WCParams);
	UEquipmentWidgetController* GetEquipmentWidgetController(const FWidgetControllerParams& WCParams);
	UInventoryWidgetController* GetInventoryWidgetController(const FWidgetControllerParams& WCParams);
private:
	UPROPERTY()
	TObjectPtr<UMageUserWidget> OverlayWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UMageUserWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UAttributeMenuWidgetController> AttributeMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAttributeMenuWidgetController> AttributeMenuWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<USkillTreeWidgetController> SkillTreeWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USkillTreeWidgetController> SkillTreeWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UEquipmentWidgetController> EquipmentWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UEquipmentWidgetController> EquipmentWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UInventoryWidgetController> InventoryWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UInventoryWidgetController> InventoryWidgetControllerClass;
};


