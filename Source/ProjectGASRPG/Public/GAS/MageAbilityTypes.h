#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "UObject/Object.h"
#include "MageAbilityTypes.generated.h"

class UAbilitySystemGlobals;
/* 继承 FGameplayEffectContext, 添加自定义数据，在MageAbilitySystemGlobals中配置 */
USTRUCT()
struct FMageGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:

	FORCEINLINE bool GetIsCriticalHit() const { return bIsCriticalHit; }
	FORCEINLINE void SetIsCriticalHit(bool bInIsCriticalHit) { bIsCriticalHit = bInIsCriticalHit; }

	
	/** 返回用于序列化的实际结构体，子类必须重载该函数 */
	virtual UScriptStruct* GetScriptStruct() const override;

	/// 自定义网络序列化，子类必须重载该函数（新添加的变量不要忘了加入到该函数中）
	/// @param Ar 保存、加载、储存、序列化数据
	/// @param Map 将对象和名字映射到索引，用于网络通信
	/// @param bOutSuccess 输出是否成功
	/// @return 
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	/** 创建此 GameplayEffectContext 的副本，用于复制以便以后修改 */
	virtual FGameplayEffectContext* Duplicate() const override;

protected:

	/** 是否暴击 */
	UPROPERTY()
	bool bIsCriticalHit = false;
};


/**
 * 定义了结构体<FMageGameplayEffectContext>可以做什么
 * 在TStructOpsTypeTraitsBase2类中可以看到所有功能，标记为true即为开启对应功能
 */ 
template<>
struct TStructOpsTypeTraits< FMageGameplayEffectContext > : public TStructOpsTypeTraitsBase2< FMageGameplayEffectContext >
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		//必须开启，用于将 TSharedPtr<FHitResult> 数据复制到各处
	};
};