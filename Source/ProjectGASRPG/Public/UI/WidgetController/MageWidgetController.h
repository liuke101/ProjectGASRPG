#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MageWidgetController.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;

/** 等级数据变化委托， 等级数据即经验值、等级、属性点、技能点等相关数据 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelDataChangedDelegate, int32, NewLevelData);

USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()
	
	FWidgetControllerParams(){}
	FWidgetControllerParams(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
		: PlayerController(PC)
		, PlayerState(PS)
		, AbilitySystemComponent(ASC)
		, AttributeSet(AS)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<APlayerController> PlayerController = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<APlayerState> PlayerState= nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent= nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_WidgetControllerParams")
	TObjectPtr<UAttributeSet> AttributeSet= nullptr;
};

UCLASS(BlueprintType, Blueprintable)
class PROJECTGASRPG_API UMageWidgetController : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetControllerParams(const FWidgetControllerParams& WCParams);

	/* 广播初始值，供 UserWidget 初始化 */
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValue();

	/* 绑定属性变化委托函数，接收属性变化 */
	virtual void BindCallbacks();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<APlayerController> PlayerController;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<APlayerState> PlayerState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;  //监听ASC
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UAttributeSet> AttributeSet;
};
