
#pragma once

#include "CoreMinimal.h"
#include "Misc/TypeContainer.h"

/**
 * 单例
 */
struct FMageGameplayTags
{
public:
 static const FMageGameplayTags& Get() {return Instance; }
 static void InitNativeGameplayTags();

private:
 // 私有构造函数，防止外部实例化
 static FMageGameplayTags Instance;
};