#pragma once

#include "CoreMinimal.h"
#include "MageWidgetController.h"
#include "OverlayWidgetController.generated.h"

struct FOnAttributeChangeData;

/* 属性变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignature, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxHealthChangedSignature, float, NewMaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManaChangedSignature, float, NewMana);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxManaChangedSignature, float, NewMaxMana);

UCLASS()
class PROJECTGASRPG_API UOverlayWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/*
	 * 广播初始值，供 OverlayUserWidget 初始化
	 * InitOverlayWidget() 中调用
	 */
	virtual void BrodCastInitialValue() override;

	/*
	 * 绑定委托回调函数
	 * GetOverlayWidgetController() 中调用
	 */
	virtual void BindCallbacks() override; 

	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnHealthChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnMaxHealthChangedSignature OnMaxHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnManaChangedSignature OnManaChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnMaxHealthChangedSignature OnMaxManaChanged;
	
protected:
	void OnHealthChangedCallback(const FOnAttributeChangeData& Data) const;
	void OnMaxHealthChangedCallback(const FOnAttributeChangeData& Data) const;
	void OnManaChangedCallback(const FOnAttributeChangeData& Data) const;
	void OnMaxManaChangedCallback(const FOnAttributeChangeData& Data) const;
};
