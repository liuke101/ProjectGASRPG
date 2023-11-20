#pragma once

#include "CoreMinimal.h"
#include "MageWidgetController.h"
#include "SkillTreeWidgetController.generated.h"

UCLASS()
class PROJECTGASRPG_API USkillTreeWidgetController : public UMageWidgetController
{
	GENERATED_BODY()

public:
	/**
     * 广播初始值，供 SkillTree 初始化
     * 在 SetWidgetController() 之后调用
     */
	virtual void BroadcastInitialValue() override;

	/**
	 * 绑定委托回调
	 * GetSkillTreeWidgetController() 中调用
	 */
	virtual void BindCallbacks() override;

	/** 等级数据变化委托，由WBP_SkillTreeMenu监听 */
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnLevelDataChangedDelegate OnSkillPointChangedDelegate;
};
