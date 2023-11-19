#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MageWidgetController.generated.h"

class UAbilityDataAsset;
class UMageAttributeSet;
class UMageAbilitySystemComponent;
class AMagePlayerState;
class AMagePlayerController;
class UAttributeSet;
class UAbilitySystemComponent;

/** 等级数据变化委托， 等级数据即经验值、等级、属性点、技能点等相关数据 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelDataChangedDelegate, int32, NewLevelData);
/** AbilityDataAsset 委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityInfoDelegate, const FMageAbilityInfo&, AbilityInfo);

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

	/*
	 * 广播初始值，供 UserWidget 初始化
	 * 在 SetWidgetController() 之后调用
	 */
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValue();

	/* 绑定属性变化委托函数，接收属性变化 */
	virtual void BindCallbacks();

	/**
	 * AbilitiesGiven 委托回调
	 * - 获取所有授予的Ability, 对每个 Ability 查询 AbilityDataAsset（获取对应的AbilityInfo）并将AbilityInfo广播给UserWidget
	 * - UserWidget通过在蓝图中绑定AbilityInfoDelegate获取AbilityInfo
	 */
	void BroadcastAbilityInfo();

	/* AbilityDataAsset 委托，由 WBP_SkillIcon(属于OverlayWidgetController) 监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FAbilityInfoDelegate AbilityInfoDelegate;

	AMagePlayerController* GetMagePlayerController();
	AMagePlayerState* GetMagePlayerState();
	UMageAbilitySystemComponent* GetMageASC();
	UMageAttributeSet* GetMageAttributeSet();
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Data")
	TObjectPtr<UAbilityDataAsset> AbilityDataAsset;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<APlayerController> PlayerController;
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<AMagePlayerController> MagePlayerController;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<APlayerState> PlayerState;
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<AMagePlayerState> MagePlayerState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;  //监听ASC
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UMageAbilitySystemComponent> MageAbilitySystemComponent;  //监听ASC
	
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UAttributeSet> AttributeSet;
	UPROPERTY(BlueprintReadOnly, Category = "Mage_WidgetController")
	TObjectPtr<UMageAttributeSet> MageAttributeSet;

	
};
