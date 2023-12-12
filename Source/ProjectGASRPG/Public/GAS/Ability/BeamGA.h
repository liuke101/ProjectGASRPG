// 

#pragma once

#include "CoreMinimal.h"
#include "MageDamageGameplayAbility.h"
#include "BeamGA.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGASRPG_API UBeamGA : public UMageDamageGameplayAbility
{
	GENERATED_BODY()
public:

	/** 从武器Socket 发射球形Trace，获取第一个Actor */
	UFUNCTION(BlueprintCallable,Category = "Mage_GA|Beam")
	void TraceFirstTarget(const FVector& TargetLocation);

	/** 存储距离最近的其他Actor */
	UFUNCTION(BlueprintCallable,Category = "Mage_GA|Beam")
	void StoreAdditionalTarget(TArray<AActor*>& OutAdditionalTargets, const int32 TargetNum = 5, const float Radius = 800.0f);

	/** OnDeath委托回调, 目标死亡时触发，在蓝图中实现 */
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable,Category = "Mage_GA|Beam")
	void OnTargetDiedCallback(AActor* DeadActor);
protected:
	UPROPERTY(EditdefaultsOnly, BlueprintReadWrite,Category = "Mage_GA|Beam")
	int32 MaxTargetNum = 5;

	
};
