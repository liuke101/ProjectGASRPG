#pragma once

#include "CoreMinimal.h"
#include "MageGA_Damage.h"
#include "MageGA_AOE.generated.h"

class AMageIceBlast;
/** 球形技能 */
UCLASS()
class PROJECTGASRPG_API UMageGA_AOE : public UMageGA_Damage
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	
	//拖拽Actor，可实现推、拉、击退等效果
	UFUNCTION(BlueprintCallable, Category = "Mage|Radius")
	void DragActor(AActor* TargetActor,FVector Direction,float ForceMagnitude ) const;
};
