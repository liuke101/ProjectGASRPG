#include "Character/GammerCharacter.h"


AGammerCharacter::AGammerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGammerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGammerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AGammerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

