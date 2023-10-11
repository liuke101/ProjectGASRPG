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
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
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

	InitAbilityActorInfo();
}

void AMageCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAbilityActorInfo();
}

void AMageCharacter::InitAbilityActorInfo()
{
	/* 该函数被PossessedBy() 和 OnRep_PlayerState()调用 */
	
	if(AMagePlayerState* MagePlayerState = GetPlayerState<AMagePlayerState>())
	{
		/*
		 * PossessedBy(): 在服务器上设置 ASC
		 * OnRep_PlayerState()：为客户端设置 ASC
		 */
		
		/*
		 * PossessedBy(): 
		 * AI 没有 PlayerController，因此我们可以在这里再次 init 以确保万无一失。
		 * 对于拥有 PlayerController 的 Character，init两次也无妨。
		 *
		 * OnRep_PlayerState():
		 * 为客户端init AbilityActorInfo
		 * 当服务器 possess 一个新的 Actor 时，它将init自己的 ASC。
		*/
		
		AbilitySystemComponent = MagePlayerState->GetAbilitySystemComponent();
		AbilitySystemComponent->InitAbilityActorInfo(MagePlayerState, this);
		Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->BindEffectCallbacks();
		
		/*
		 * 初始化 AttributeSet
		 *
		 * PlayerState 中的 AttributeSet类为 MageAttributeSet，在初始化 OverlayWidget 时传入在 OverlayWidget 中可以直接转为 MageAttributeSet 使用
		 */
		AttributeSet = MagePlayerState->GetAttributeSet(); 

		/* 初始化 OverlayWidget */
		if(AMagePlayerController* MagePlayerController = Cast<AMagePlayerController>(GetController()))
		{
			if(AMageHUD* MageHUD = Cast<AMageHUD>(MagePlayerController->GetHUD()))
			{
				MageHUD->InitOverlayWidget(MagePlayerController, MagePlayerState, AbilitySystemComponent, AttributeSet);
			}
		}

		/* 初始化主要属性 */
		InitPrimaryAttributes();
	}
}
