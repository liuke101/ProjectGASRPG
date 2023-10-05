#include "ProjectGASRPG/Public/Character/GammerCharacterBase.h"

AGammerCharacterBase::AGammerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponHandSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGammerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

