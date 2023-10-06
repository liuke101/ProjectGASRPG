#include "Character/MageEnemy.h"

#include "Components/CapsuleComponent.h"
#include "ProjectGASRPG/ProjectGASRPG.h"


AMageEnemy::AMageEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
}

void AMageEnemy::BeginPlay()
{
	Super::BeginPlay();
	
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
	GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_STENCIL_VALUE);
	WeaponMesh->SetRenderCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_STENCIL_VALUE);
}

void AMageEnemy::UnHighlightActor()
{
	GetMesh()->SetRenderCustomDepth(false);
	WeaponMesh->SetRenderCustomDepth(false);
}

