#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "UObject/Object.h"
#include "MageAbilityTypes.generated.h"

/* 继承 FGameplayEffectContext, 添加自定义数据 */
USTRUCT()
struct FMageGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:

	FORCEINLINE bool GetIsCriticalHit() const { return bIsCriticalHit; }
	FORCEINLINE void SetIsCriticalHit(bool bInIsCriticalHit) { bIsCriticalHit = bInIsCriticalHit; }

	
	/** 返回用于序列化的实际结构体，子类必须重载该函数 */
	virtual UScriptStruct* GetScriptStruct() const override;

	/** 自定义序列化，子类必须重载该函数 */
	/** 新添加的变量不要忘了加入到该函数中 */
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

protected:

	/** 是否暴击 */
	UPROPERTY()
	bool bIsCriticalHit = false;
};

