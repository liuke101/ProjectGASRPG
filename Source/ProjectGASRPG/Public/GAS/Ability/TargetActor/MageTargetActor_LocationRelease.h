// 

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MageTargetActor_LocationRelease.generated.h"

class AMagicCircle;

UCLASS()
class PROJECTGASRPG_API AMageTargetActor_LocationRelease : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	AMageTargetActor_LocationRelease();
	
	virtual void Tick(float DeltaTime) override;
public:
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	
	

	
	// 获取视线与地面的碰撞点
	UFUNCTION(BlueprintCallable, Category="Chikabu Tensei")
	bool GetPlayerLookingPoint(FVector& OutLookingPoint);

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Settings", meta=(ExposeOnSpawn = true))
	TSubclassOf<AMagicCircle> MagicCircleClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Settings", meta=(ExposeOnSpawn = true))
	TSubclassOf<AActor> AbilityActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability Settings", meta=(ExposeOnSpawn = true))
	float Radius;
protected:
	UPROPERTY()
	FVector LookingPoint;
	//技能范围指示器
	UPROPERTY()
	AMagicCircle* MagicCircle;

	//表示技能产生的actor
	UPROPERTY()
	AActor* AbilityActor;
};
