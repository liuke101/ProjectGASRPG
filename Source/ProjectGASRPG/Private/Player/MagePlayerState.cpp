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
	DOREPLIFETIME(AMagePlayerState, EXP);
	DOREPLIFETIME(AMagePlayerState, CharacterClass);
}

void AMagePlayerState::AddToLevel(int32 InLevel)
{
	Level += InLevel;
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::AddToEXP(int32 InEXP)
{
	EXP += InEXP;
	OnPlayerEXPChanged.Broadcast(EXP);
}

void AMagePlayerState::SetEXP(int32 InEXP)
{
	EXP = InEXP;
	OnPlayerEXPChanged.Broadcast(EXP);
}

void AMagePlayerState::OnRep_Level(int32 OldData) 
{
	OnPlayerLevelChanged.Broadcast(Level);
}

void AMagePlayerState::OnRep_EXP(int32 OldData)
{
	OnPlayerEXPChanged.Broadcast(EXP);
}

void AMagePlayerState::OnRep_CharacterClass(ECharacterClass OldCharacterClass)
{
}
