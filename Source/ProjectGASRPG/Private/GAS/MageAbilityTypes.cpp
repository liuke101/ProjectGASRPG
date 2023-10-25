#include "GAS/MageAbilityTypes.h"

UScriptStruct* FMageGameplayEffectContext::GetScriptStruct() const
{
	return FGameplayEffectContext::GetScriptStruct();
}

bool FMageGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	return FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);
}
