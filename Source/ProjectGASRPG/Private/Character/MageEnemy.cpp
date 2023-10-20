#include "Character/MageEnemy.h"

#include "Components/CapsuleComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "ProjectGASRPG/ProjectGASRPG.h"


AMageEnemy::AMageEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	AbilitySystemComponent = CreateDefaultSubobject<UMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<UMageAttributeSet>(TEXT("AttributeSet"));
}

void AMageEnemy::BeginPlay()
{
	Super::BeginPlay();

	InitAbilityActorInfo();
}

void AMageEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 
}

void AMageEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AMageEnemy::HighlightActor()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(CustomDepthStencilValue);
	Weapon->SetRenderCustomDepth(true);
	Weapon->SetCustomDepthStencilValue(CustomDepthStencilValue);
}

void AMageEnemy::UnHighlightActor()
{
	GetMesh()->SetRenderCustomDepth(false);
	Weapon->SetRenderCustomDepth(false);
}

void AMageEnemy::InitAbilityActorInfo()
{
	// 初始化ASC
	if(AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->BindEffectCallbacks();

	InitDefaultAttributes();
}

