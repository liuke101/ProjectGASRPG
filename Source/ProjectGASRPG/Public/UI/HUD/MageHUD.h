#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MageHUD.generated.h"

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
	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	USkillTreeWidgetController* GetSkillTreeWidgetController(const FWidgetControllerParams& WCParams);
	/*
	 * 初始化 OverlayWidget
	 * 在 MageCharater类 InitASCandAS() 中调用
	 */
	void InitOverlayWidget(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
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
};


