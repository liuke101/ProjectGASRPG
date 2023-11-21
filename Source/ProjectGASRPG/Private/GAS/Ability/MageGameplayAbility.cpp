#include "GAS/Ability/MageGameplayAbility.h"

UMageGameplayAbility::UMageGameplayAbility()
{
	bServerRespectsRemoteAbilityCancellation = false;
}

FString UMageGameplayAbility::GetDescription(int32 AbilityLevel)
{
	return FString::Printf(TEXT("<MainBody>%s</>\n<MainBody>技能等级：</><Level>%d</>"),L"test test test test test test test test test test test test test test test test test test test test test",AbilityLevel);
}

FString UMageGameplayAbility::GetNextLevelDescription(int32 AbilityLevel)
{
	return FString::Printf(TEXT("<MainBody>下一级：</><Level>%d</>\n<MainBody>造成更多伤害。</>"),AbilityLevel+1);
}

FString UMageGameplayAbility::GetLockedDescription(int32 LevelRequirement)
{
	return FString::Printf(TEXT("<MainBody>技能解锁等级：</><Level>%d</>"),LevelRequirement);
}


