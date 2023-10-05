#include "ProjectGASRPG/Public/Character/GammerCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"

AGammerCharacterBase::AGammerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; //朝向旋转到移动方向
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponHandSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGammerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

