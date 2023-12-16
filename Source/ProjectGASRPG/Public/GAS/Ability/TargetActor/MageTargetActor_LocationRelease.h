// 

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MageTargetActor_LocationRelease.generated.h"

UCLASS()
class PROJECTGASRPG_API AMageTargetActor_LocationRelease : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	AMageTargetActor_LocationRelease();

public:
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability Settings", meta=(ExposeOnSpawn = true))
	float Radius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Settings", meta=(ExposeOnSpawn = true))
	TSubclassOf<AActor> ConePointClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Settings", meta=(ExposeOnSpawn = true))
	TSubclassOf<AActor> MeteorClass;
	
	// 获取实线与地面的碰撞点
	UFUNCTION(BlueprintCallable, Category="Chikabu Tensei")
	bool GetPlayerLookingPoint(FVector& LookingPoint);

protected:
	//表示技能释放位置
	UPROPERTY()
	AActor* ConePoint;

	//表示技能产生的actor
	UPROPERTY()
	AActor* Meteor;
};
