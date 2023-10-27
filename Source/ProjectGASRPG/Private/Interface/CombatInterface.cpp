#include "Interface/CombatInterface.h"

int32 ICombatInterface::GetCharacterLevel() const
{
	return 0;
}

ECharacterClass ICombatInterface::GetCharacterClass() const
{
	return ECharacterClass::None;
}

FVector ICombatInterface::GetWeaponSocketLocation()
{
	return FVector::ZeroVector;
}
