#include "Player/MagePlayerState.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "Net/UnrealNetwork.h"

AMagePlayerState::AMagePlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	
	AttributeSet = CreateDefaultSubobject<UMageAttributeSet>(TEXT("AttributeSet"));
}

void AMagePlayerState::SetLevel(const int32 InLevel)
{
	Level = InLevel;
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::AddToLevel(const int32 InLevel)
{
	Level += InLevel;
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::SetExp(const int32 InExp)
{
	Exp = InExp;
	OnPlayerExpChanged.Broadcast(Exp);
}

void AMagePlayerState::AddToExp(const int32 InExp)
{
	Exp += InExp;
	OnPlayerExpChanged.Broadcast(Exp);
}

void AMagePlayerState::SetAttributePoint(const int32 InAttributePoint)
{
	AttributePoint = InAttributePoint;
	OnPlayerAttributePointChanged.Broadcast(AttributePoint);
}

void AMagePlayerState::AddToAttributePoint(const int32 InAttributePoint)
{
	AttributePoint += InAttributePoint;
	OnPlayerAttributePointChanged.Broadcast(AttributePoint);
}

void AMagePlayerState::SetSkillPoint(const int32 InSkillPoint)
{
	SkillPoint = InSkillPoint;
	OnPlayerSkillPointChanged.Broadcast(SkillPoint);
}

void AMagePlayerState::AddToSkillPoint(const int32 InSkillPoint)
{
	SkillPoint += InSkillPoint;
	OnPlayerSkillPointChanged.Broadcast(SkillPoint);
}


