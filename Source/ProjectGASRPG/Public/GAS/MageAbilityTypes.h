#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "UObject/Object.h"
#include "MageAbilityTypes.generated.h"

class UGameplayEffect;
class UAbilitySystemGlobals;

USTRUCT(BlueprintType)
struct FDamageEffectParams
{
	GENERATED_BODY()

	FDamageEffectParams(){}

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> WorldContextObject = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> SourceASC;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetASC;

	UPROPERTY(BlueprintReadWrite)
	float AbilityLevel = 1.f;
	
	UPROPERTY(BlueprintReadWrite)
	float BaseDamage = 0.f;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag DamageTypeTag = FGameplayTag();

	UPROPERTY(BlueprintReadWrite)
	float DebuffChance = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float DebuffDamage = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float DebuffFrequency = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float DebuffDuration = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float DeathImpulseMagnitude = 0.f;

	UPROPERTY(BlueprintReadWrite)
	FVector DeathImpulse = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float KnockbackForceMagnitude = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float KnockbackChance = 0.f;

	UPROPERTY(BlueprintReadWrite)
	FVector KnockbackForce = FVector::ZeroVector;
};

/* 继承 FGameplayEffectContext, 添加自定义数据，在MageAbilitySystemGlobals中配置 */
USTRUCT(BlueprintType)
struct FMageGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	/** Getter/Setter */
	FORCEINLINE bool GetIsCriticalHit() const { return bIsCriticalHit; }
	FORCEINLINE void SetIsCriticalHit(const bool InbIsCriticalHit) { bIsCriticalHit = InbIsCriticalHit; }

	FORCEINLINE bool GetIsDebuff() const { return bIsDebuff; }
	FORCEINLINE void SetIsDebuff(const bool InbIsDebuff) { bIsDebuff = InbIsDebuff; }
	
	FORCEINLINE float GetDebuffDamage() const { return DebuffDamage; }
	FORCEINLINE void SetDebuffDamage(const float InDebuffDamage) { DebuffDamage = InDebuffDamage; }
	
	FORCEINLINE float GetDebuffFrequency() const { return DebuffFrequency; }
	FORCEINLINE void SetDebuffFrequency(const float InDebuffFrequency) { DebuffFrequency = InDebuffFrequency; }
	
	FORCEINLINE float GetDebuffDuration() const { return DebuffDuration; }
	FORCEINLINE void SetDebuffDuration(const float InDebuffDuration) { DebuffDuration = InDebuffDuration; }

	FORCEINLINE TSharedPtr<FGameplayTag> GetDamageTypeTag() const { return DamageTypeTag; }
	FORCEINLINE void SetDamageTypeTag(const TSharedPtr<FGameplayTag>& InDamageTypeTag) { DamageTypeTag = InDamageTypeTag; }

	FORCEINLINE FVector GetDeathImpulse() const { return DeathImpulse; }
	FORCEINLINE void SetDeathImpulse(const FVector& InDeathImpulse) { DeathImpulse = InDeathImpulse; }
	
	FORCEINLINE FVector GetKnockbackForce() const { return KnockbackForce; }
	FORCEINLINE void SetKnockbackForce(const FVector& InKnockbackForce) { KnockbackForce = InKnockbackForce; }
	
	/// 自定义网络序列化，子类必须重载该函数（新添加的变量不要忘了加入到该函数中）
	/// @param Ar 保存、加载、储存、序列化数据
	/// @param Map 将对象和名字映射到索引，用于网络通信
	/// @param bOutSuccess 输出是否成功
	/// @return 
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	/** 返回用于序列化的实际结构体，子类必须重载该函数 */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	/** 创建此 GameplayEffectContext 的副本，用于复制以便以后修改 */
	virtual FMageGameplayEffectContext* Duplicate() const override
	{
		FMageGameplayEffectContext* NewContext = new FMageGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

protected:

	/** 是否暴击 */
	UPROPERTY()
	bool bIsCriticalHit = false;

	/** 是否是debuff */
	UPROPERTY()
	bool bIsDebuff = false;

	UPROPERTY()
	float DebuffDamage = 0.f;

	UPROPERTY()
	float DebuffFrequency = 0.f;

	UPROPERTY()
	float DebuffDuration = 0.f;

	TSharedPtr<FGameplayTag> DamageTypeTag = nullptr;

	UPROPERTY()
	FVector DeathImpulse = FVector::ZeroVector;

	UPROPERTY()
	FVector KnockbackForce = FVector::ZeroVector;
};


/**
 * 定义了结构体<FMageGameplayEffectContext>可以做什么
 * 在TStructOpsTypeTraitsBase2类中可以看到所有功能，标记为true即为开启对应功能
 */ 
template<>
struct TStructOpsTypeTraits<FMageGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FMageGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		//必须开启，用于将 TSharedPtr<FHitResult> 数据复制到各处
	};
};