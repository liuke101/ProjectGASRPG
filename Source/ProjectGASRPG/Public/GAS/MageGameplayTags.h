#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Misc/TypeContainer.h"

/**
  * 单例类
  * 
  * 访问GameplayTag的方法：
  * 1. 获取单例：const FMageGameplayTags& GameplayTagsInstance = FMageGameplayTags::Get();
  * 2. 通过单例访问FGameplayTag成员变量：GameplayTagsInstance.Attribute_Secondary_MaxHealth
  */
class FMageGameplayTags
{
public:
	static const FMageGameplayTags& Get() { return GameplayTagsInstance; } // 单例

	/* 通过C++添加Native GameplayTag, 不再使用ini或数据表配置Tag */
	static void InitNativeGameplayTags();


#pragma region "GameplayTags"
	FGameplayTag Attribute_Secondary_MaxHealth;
#pragma endregion

private:
	static FMageGameplayTags GameplayTagsInstance;
};
