#include "Character/MageCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


AMageCharacter::AMageCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->bOrientRotationToMovement = false; //朝向旋转到移动方向，开启：后退转向，关闭：后退不转向
	GetCharacterMovement()->bUseControllerDesiredRotation = true; //使用控制器的旋转

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f; 
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	SpringArm->SetRelativeRotation(FRotator(-40.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false; 
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	FollowCamera->SetupAttachment(SpringArm); 
	FollowCamera->bUsePawnControlRotation = false; 
}

void AMageCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMageCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

