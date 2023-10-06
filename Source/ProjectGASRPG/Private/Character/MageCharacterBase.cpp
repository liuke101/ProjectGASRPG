#include "ProjectGASRPG/Public/Character/MageCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"

AMageCharacterBase::AMageCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	//Controller 旋转时不跟着旋转。让它只影响Camera。
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; //朝向旋转到移动方向，开启：后退转向，关闭：后退不转向
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponHandSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMageCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* AMageCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* AMageCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

