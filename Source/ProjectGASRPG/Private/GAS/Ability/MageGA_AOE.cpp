#include "GAS/Ability/MageGA_AOE.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/Ability/Actor/MageIceBlast.h"
#include "Interface/CombatInterface.h"
#include "Interface/EnemyInterface.h"
#include "Kismet/KismetSystemLibrary.h"

void UMageGA_AOE::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                     const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}



void UMageGA_AOE::DragActor(AActor* TargetActor, FVector Direction, float ForceMagnitude) const
{
	if(const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		TargetCharacter->GetCharacterMovement()->AddImpulse(Direction * ForceMagnitude, true);
	}
}
