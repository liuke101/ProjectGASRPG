// 

#pragma once

#include "CoreMinimal.h"
#include "MageWidgetController.h"
#include "EquipmentWidgetController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UEquipmentWidgetController : public UMageWidgetController
{
	GENERATED_BODY()
public:
	/**
	 * 广播初始值，供 EquipmentWidget 初始化
	 * 在蓝图中 SetWidgetController() 之后调用
	 */
	virtual void BroadcastInitialValue() override;
	
	/**
	 * 绑定委托回调
	 * GetAttributeMenuWidgetController() 中调用
	 */
	virtual void BindCallbacks() override;
};
