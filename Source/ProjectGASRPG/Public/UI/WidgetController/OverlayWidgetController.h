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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignature, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChangedSignature, float, NewMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManaChangedSignature, float, NewMana);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxManaChangedSignature, float, NewMaxMana);

/** 数据表委托，由UserWidget接收 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageWidgetRowSignature, FUIWidgetRow, NewMessageWidgetRow);

UCLASS()
class PROJECTGASRPG_API UOverlayWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/**
	 * 广播初始值，供 OverlayUserWidget 初始化
	 * InitOverlayWidget() 中调用
	 */
	virtual void BrodCastInitialValue() override;

	/**
	 * 绑定委托回调函数
	 * GetOverlayWidgetController() 中调用
	 */
	virtual void BindCallbacks() override; 

	/* 声明委托对象 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnHealthChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnMaxHealthChangedSignature OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnManaChangedSignature OnManaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnMaxHealthChangedSignature OnMaxManaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FMessageWidgetRowSignature MessageWidgetRowDelegate;
	

	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Data")
	TObjectPtr<UDataTable> MessageWidgetDataTable;
	
	void OnHealthChangedCallback(const FOnAttributeChangeData& Data) const;
	void OnMaxHealthChangedCallback(const FOnAttributeChangeData& Data) const;
	void OnManaChangedCallback(const FOnAttributeChangeData& Data) const;
	void OnMaxManaChangedCallback(const FOnAttributeChangeData& Data) const;

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

