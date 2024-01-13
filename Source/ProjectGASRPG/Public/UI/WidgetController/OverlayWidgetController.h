#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MageWidgetController.h"
#include "Engine/DataTable.h"
#include "OverlayWidgetController.generated.h"

class AMageItem;
class UMageAbilitySystemComponent;
class UAbilityDataAsset;
class UMageUserWidget;
struct FGameplayTag;
struct FOnAttributeChangeData;

/** 数据表 */
USTRUCT(BlueprintType)
struct FUIWidgetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag MessageTag = FGameplayTag();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Message = FText();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UMageUserWidget> MessageWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Image = nullptr;
};

/** 数据表委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageWidgetRowDelegate, FUIWidgetRow, NewMessageWidgetRow);
/** 经验值变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExpChangedDelegate, int32, CurrentValue, int32, MaxValue);
/** InteractableData 委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractableDataDelegate, const FInteractableData&, InteractableData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHideInteractionWidgetDelegate);

UCLASS()
class PROJECTGASRPG_API UOverlayWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/**
	 * 绑定委托回调
	 * GetOverlayWidgetController() 中调用
	 */
	virtual void BindCallbacks() override;
	
	/**
	 * 广播初始值，供 OverlayUserWidget 初始化
	 * 在 InitOverlayWidget() 中SetWidgetController()之后调用
	 */
	virtual void BroadcastInitialValue() override;

	/* 数据表委托，由 WBP_OverlayWidget 监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FMessageWidgetRowDelegate MessageWidgetRowDelegate;

	/** 拾取物品信息委托，由 WBP_OverlayWidget 监听 */
	// UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	

	/** 经验值变化委托，由 WBP_ExperienceBar 监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnExpChangedDelegate OnExpChangedDelegate;

	/** 等级数据变化委托，由 WBP_ExperienceBar 监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnLevelChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnAttributePointChangedDelegate;

	/** InteractableData 委托，由 WBP_InteractionWidget 监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FInteractableDataDelegate OnInteractableDataChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FHideInteractionWidgetDelegate HideInteractionWidget;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Data")
	TObjectPtr<UDataTable> MessageWidgetDataTable;

	/** 根据 GameplayTag 获取数据表行 */
	template<typename T>
	T* GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag) const;

	/** 经验值变化回调 */
	void OnExpChangedCallback(const int32 NewExp);

	/** 广播AbilityInfo更新Icon, 由WBP_SkillIcon监听 */
	void OnSkillEquippedCallback(const FGameplayTag& AbilityTag, const FGameplayTag& AbilityStateTag, const FGameplayTag& SlotInputTag, const FGameplayTag& PreSlotInputTag) const;

};

template <typename T>
T* UOverlayWidgetController::GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag) const
{
	T* Row = DataTable->FindRow<T>(Tag.GetTagName(), TEXT(""));
	return Row;
}

