#include "Character/MageEnemy.h"


AMageEnemy::AMageEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
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

