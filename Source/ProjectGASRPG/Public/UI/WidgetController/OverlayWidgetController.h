#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MageWidgetController.h"
#include "Engine/DataTable.h"
#include "OverlayWidgetController.generated.h"

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

/** 属性变化委托，由UserWidget接收 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);
/** 数据表委托，由UserWidget接收 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageWidgetRowSignature, FUIWidgetRow, NewMessageWidgetRow);

UCLASS()
class PROJECTGASRPG_API UOverlayWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/**
	 * 广播初始值，供 OverlayUserWidget 初始化
	 * 在 InitOverlayWidget() 中SetWidgetController()之后调用
	 */
	virtual void BroadcastInitialValue() override;

	/**
	 * 绑定委托回调
	 * GetOverlayWidgetController() 中调用
	 */
	virtual void BindCallbacks() override; 

	/* 声明委托对象 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeChangedSignature OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeChangedSignature OnManaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeChangedSignature OnMaxManaChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FMessageWidgetRowSignature MessageWidgetRowDelegate;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Data")
	TObjectPtr<UDataTable> MessageWidgetDataTable;
	
	/** 根据 GameplayTag 获取数据表行 */
	template<typename T>
	T* GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag) const;
};

template <typename T>
T* UOverlayWidgetController::GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag) const
{
	T* Row = DataTable->FindRow<T>(Tag.GetTagName(), TEXT(""));
	return Row;
}

