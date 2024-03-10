#pragma once

#include "CoreMinimal.h"
#include "MageGA_Damage.h"
#include "MageGA_AOE.generated.h"

class AMageIceBlast;

UCLASS()
class PROJECTGASRPG_API UMageGA_AOE : public UMageGA_Damage
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
