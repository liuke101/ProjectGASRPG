#include "GAS/Ability/MageGameplayAbility.h"

UMageGameplayAbility::UMageGameplayAbility()
{
	bServerRespectsRemoteAbilityCancellation = false;
}

FString UMageGameplayAbility::GetDescription_Implementation(const int32 AbilityLevel)
{
	return FString::Printf(TEXT("<MainBody>%s</>\n<MainBody>技能等级：</><Level>%d</>"),L"test",AbilityLevel);
}

FString UMageGameplayAbility::GetNextLevelDescription_Implementation(const int32 AbilityLevel)
{
	return GetDescription(AbilityLevel+1);
}

FString UMageGameplayAbility::GetLockedDescription(const int32 LevelRequirement)
{
	return FString::Printf(TEXT("<MainBody>技能解锁等级：</><Level>%d</>"),LevelRequirement);
}




