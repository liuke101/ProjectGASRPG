#pragma once

#include "CoreMinimal.h"
#include "Inventory/Data/ItemDataAsset.h"
#include "UObject/Object.h"
#include "MageWidgetController.generated.h"

class AMageItem;
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
/** ItemDataAsset委托 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMageItemInfoDelegate, const AMageItem* Item, const FMageItemInfo& MageItemInfo);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState= nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent= nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAttributeSet> AttributeSet= nullptr;
};

UCLASS(BlueprintType, Blueprintable)
class PROJECTGASRPG_API UMageWidgetController : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetWidgetControllerParams(const FWidgetControllerParams& WCParams);

	/* 绑定属性变化委托函数，接收属性变化 */
	virtual void BindCallbacks();
	
	/*
	 * 广播初始值，供 UserWidget 初始化
	 * 在 SetWidgetController() 之后调用
	 */
	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValue();

	/**
	 * AbilitiesGiven 委托回调
	 * - 获取所有授予的Ability, 对每个 Ability 查询 AbilityDataAsset（获取对应的AbilityInfo）并将AbilityInfo广播给UserWidget
	 * - UserWidget通过在蓝图中绑定AbilityInfoDelegate获取AbilityInfo
	 */
	void BroadcastAbilityInfo();

	/* AbilityDataAsset 委托，由 WBP_SkillIcon(属于OverlayWidgetController) 监听 */
	UPROPERTY(BlueprintAssignable, Category = "MageWidgetController|Delegates")
	FAbilityInfoDelegate AbilityInfoDelegate;

	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	AMagePlayerController* GetMagePlayerController();
	
	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	AMagePlayerState* GetMagePlayerState();
	
	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	UMageAbilitySystemComponent* GetMageASC();
	
	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	UMageAttributeSet* GetMageAttributeSet();
	
	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	AActor* GetAvatarActor() const;
	
	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	AActor* GetOwnerActor() const;

	/** 物品拾取 */
	
	UFUNCTION(BlueprintPure, Category = "MageWidgetController")
	FORCEINLINE AMageItem* GetMageItem() const { return MageItem; }
	
	UFUNCTION(BlueprintCallable, Category = "MageWidgetController")
	void SetMageItem(AMageItem* InMageItem);
	
	FMageItemInfoDelegate OnSetMageItemInfo;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageWidgetController|Data")
	TObjectPtr<UAbilityDataAsset> AbilityDataAsset;
	
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<APlayerController> PlayerController;
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<AMagePlayerController> MagePlayerController;
	
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<APlayerState> PlayerState;
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<AMagePlayerState> MagePlayerState;
	
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;  //监听ASC
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<UMageAbilitySystemComponent> MageAbilitySystemComponent;  //监听ASC
	
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<UAttributeSet> AttributeSet;
	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<UMageAttributeSet> MageAttributeSet;

	UPROPERTY(BlueprintReadOnly, Category = "MageWidgetController")
	TObjectPtr<AMageItem> MageItem;
};
