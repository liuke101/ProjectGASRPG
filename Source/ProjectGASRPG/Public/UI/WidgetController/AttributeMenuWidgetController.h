﻿#pragma once

#include "CoreMinimal.h"
#include "MageWidgetController.h"
#include "AttributeMenuWidgetController.generated.h"

struct FGameplayTag;
class UAttributeDataAsset;
struct FMageAttributeInfo;

/** AttributeInfo委托，BP_AttributeRow 接收 AttributeInfo（数据资产）信息并更新UI */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeInfoSignature, const FMageAttributeInfo&, NewAttributeInfo);

UCLASS()
class PROJECTGASRPG_API UAttributeMenuWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/**
	 * 广播初始值，供 AttributeMenuWidget 初始化
	 * 在蓝图中 SetWidgetController() 之后调用
	 */
	virtual void BroadcastInitialValue() override;

	/** 广播AttributeInfo */
	virtual void BroadcastAttributeInfo(const FGameplayTag& GameplayTag, const float AttributeCurrentValue);

	/**
	 * 绑定委托回调
	 * GetAttributeMenuWidgetController() 中调用
	 */
	virtual void BindCallbacks() override;

	/** 声明委托对象 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeInfoSignature AttributeInfoDelegate;

	/** 等级数据变化委托，由 WBP_ExperienceBar 监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnLevelChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnAttributePointChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnSkillPointChangedDelegate;

	/** 根据GameplayTag升级属性 */
	UFUNCTION(BlueprintCallable, Category = "Mage_AttributeMenu")
	void UpgradeAttribute(const FGameplayTag& AttributeTag);

protected:
	/** 数据资产DataAsset类 */
	UPROPERTY(EditAnywhere, Category = "Mage_AttributeMenu")
	TObjectPtr<UAttributeDataAsset> AttributeDataAsset;
};
