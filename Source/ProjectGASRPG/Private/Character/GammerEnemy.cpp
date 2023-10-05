#include "Character/GammerEnemy.h"


AGammerEnemy::AGammerEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGammerEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGammerEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 
}

void AGammerEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

