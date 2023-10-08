#include "Character/MageCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "Player/MagePlayerController.h"
#include "Player/MagePlayerState.h"
#include "UI/HUD/MageHUD.h"


AMageCharacter::AMageCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->bOrientRotationToMovement = false; //朝向旋转到移动方向，开启：后退转向，关闭：后退不转向
	GetCharacterMovement()->bUseControllerDesiredRotation = true; //使用控制器的旋转, 人物始终跟随镜头转向

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	SpringArm->SetRelativeRotation(FRotator(-40.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
}

void AMageCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMageCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}


USpringArmComponent* AMageCharacter::GetSpringArm()
{
	return SpringArm;
}

void AMageCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitASCandAS();
}

void AMageCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitASCandAS();
}

void AMageCharacter::InitASCandAS()
{
	/* 该函数被PossessedBy() 和 OnRep_PlayerState()调用 */
	
	if(AMagePlayerState* MagePlayerState = GetPlayerState<AMagePlayerState>())
	{
		/*
		 * PossessedBy(): 在服务器上设置 ASC
		 * OnRep_PlayerState()：为客户端设置 ASC
		 */
		AbilitySystemComponent = Cast<UMageAbilitySystemComponent>(MagePlayerState->GetAbilitySystemComponent());

		/*
		 * PossessedBy(): 
		 * AI 没有 PlayerController，因此我们可以在这里再次 init 以确保万无一失。
		 * 对于拥有 PlayerController 的 Character，init两次也无妨。
		 *
		 * OnRep_PlayerState():
		 * 为客户端init AbilityActorInfo
		 * 当服务器 possess 一个新的 Actor 时，它将init自己的 ASC。
		*/
		AbilitySystemComponent->InitAbilityActorInfo(MagePlayerState, this);

		/* 初始化 AttributeSet */
		AttributeSet = MagePlayerState->GetAttributeSet();

		/* 初始化 OverlayWidget */
		if(AMagePlayerController* MagePlayerController = Cast<AMagePlayerController>(GetController()))
		{
			if(AMageHUD* MageHUD = Cast<AMageHUD>(MagePlayerController->GetHUD()))
			{
				MageHUD->InitOverlayWidget(MagePlayerController, MagePlayerState, AbilitySystemComponent, AttributeSet);
			}
		}
	}
}
