#include "GAS/Ability/MageGA_Radius.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/CombatInterface.h"
#include "Interface/EnemyInterface.h"

void UMageGA_Radius::DragActor(AActor* TargetActor, FVector Direction, float ForceMagnitude) const
{
	if(const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		TargetCharacter->GetCharacterMovement()->AddImpulse(Direction * ForceMagnitude, true);
	}
}
