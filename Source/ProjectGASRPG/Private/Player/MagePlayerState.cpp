#include "Player/MagePlayerState.h"

#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "Net/UnrealNetwork.h"

AMagePlayerState::AMagePlayerState()
{
	NetUpdateFrequency = 100.0f;

	AbilitySystemComponent = CreateDefaultSubobject<UMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	AttributeSet = CreateDefaultSubobject<UMageAttributeSet>(TEXT("AttributeSet"));
}

void AMagePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/** 列举复制变量 */
	DOREPLIFETIME(AMagePlayerState, Level);
	DOREPLIFETIME(AMagePlayerState, Exp);
	DOREPLIFETIME(AMagePlayerState, AttributePoint);
	DOREPLIFETIME(AMagePlayerState, SkillPoint);
	DOREPLIFETIME(AMagePlayerState, CharacterClass);
}

void AMagePlayerState::AddToLevel(const int32 InLevel)
{
	Level += InLevel;
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::SetLevel(const int32 InLevel)
{
	Level = InLevel;
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::AddToExp(const int32 InExp)
{
	Exp += InExp;
	OnPlayerExpChanged.Broadcast(Exp);
}

void AMagePlayerState::SetExp(const int32 InExp)
{
	Exp = InExp;
	OnPlayerExpChanged.Broadcast(Exp);
}

void AMagePlayerState::AddToAttributePoint(const int32 InAttributePoint)
{
	AttributePoint += InAttributePoint;
	OnPlayerAttributePointChanged.Broadcast(AttributePoint);
}

void AMagePlayerState::SetAttributePoint(const int32 InAttributePoint)
{
	AttributePoint = InAttributePoint;
	OnPlayerExpChanged.Broadcast(AttributePoint);
}

void AMagePlayerState::AddToSkillPoint(const int32 InSkillPoint)
{
	SkillPoint += InSkillPoint;
	OnPlayerSkillPointChanged.Broadcast(SkillPoint);
}

void AMagePlayerState::SetSkillPoint(const int32 InSkillPoint)
{
	SkillPoint = InSkillPoint;
	OnPlayerSkillPointChanged.Broadcast(SkillPoint);
}

void AMagePlayerState::OnRep_Level(const int32 OldData) 
{
	OnPlayerLevelChanged.Broadcast(OldData);
}

void AMagePlayerState::OnRep_Exp(const int32 OldData)
{
	OnPlayerExpChanged.Broadcast(OldData);
}

void AMagePlayerState::OnRep_AttributePoint(const int32 OldData)
{
	OnPlayerAttributePointChanged.Broadcast(OldData);
}

void AMagePlayerState::OnRep_SkillPoint(const int32 OldData)
{
	OnPlayerSkillPointChanged.Broadcast(OldData);
}

void AMagePlayerState::OnRep_CharacterClass(const ECharacterClass OldCharacterClass)
{
}
