#include "Player/MagePlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/MageCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interface/EnemyInterface.h"

AMagePlayerController::AMagePlayerController()
{
	bReplicates = true;
}

void AMagePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
}

void AMagePlayerController::BeginPlay()
{
	Super::BeginPlay();

	//添加InputMappingContext
	if (InputMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI CustomInputMode;
	CustomInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	CustomInputMode.SetHideCursorDuringCapture(false);
	SetInputMode(CustomInputMode);
}

void AMagePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 绑定Action
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		//Move: WASD
		if(MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMagePlayerController::Move);
		}

		//Look: 按住鼠标右键
		if(LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMagePlayerController::Look);
		}

		//LookAround: 按住Alt+鼠标右键
		if(LookAroundAction)
		{
			EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Started, this, &AMagePlayerController::LookAroundStart);
			EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Completed, this, &AMagePlayerController::LookAroundEnd);
		}

		//CameraZoom: 鼠标滚轮
		if(CameraZoomAction)
		{
			EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AMagePlayerController::CameraZoom);
		}
		
		
	}
}

void AMagePlayerController::Move(const FInputActionValue& InputActionValue)
{
	// input is a Vector2D
	FVector2D MovementVector = InputActionValue.Get<FVector2D>(); //X轴对应左右，Y轴对应前后

	// find out which way is forward
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector, 对应的是X轴
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); //这里的轴是世界坐标系轴

	// get right vector, 对应的是Y轴
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// add movement 
	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void AMagePlayerController::Look(const FInputActionValue& InputActionValue)
{
	// input is a Vector2D
	FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();  //X轴对应左右（Yaw），Y轴对应上下（Pitch）

	// add yaw and pitch input to controller
	GetPawn()->AddControllerYawInput(LookAxisVector.X);
	GetPawn()->AddControllerPitchInput(LookAxisVector.Y);
}

void AMagePlayerController::LookAroundStart()
{
	GetCharacter()->GetCharacterMovement()->bUseControllerDesiredRotation= false;
}

void AMagePlayerController::LookAroundEnd()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if(AMageCharacter* MageCharacter = Cast<AMageCharacter>(GetCharacter()))
	{
		FRotator SpringArmRotation = MageCharacter->GetSpringArm()->GetComponentRotation();
		FRotator CurrentControlRotation = PlayerController->GetControlRotation();
		//将SpringArm的位置和朝向匹配到当前Character的朝向
		PlayerController->SetControlRotation(FRotator(CurrentControlRotation.Pitch, SpringArmRotation.Yaw,CurrentControlRotation.Roll));
	}
	
	GetCharacter()->GetCharacterMovement()->bUseControllerDesiredRotation= true;
}

void AMagePlayerController::CameraZoom(const FInputActionValue& InputActionValue)
{
	// input is a Vector2D
	float ZoomAxis = InputActionValue.Get<float>(); 
	
	if(AMageCharacter* MageCharacter = Cast<AMageCharacter>(GetCharacter()))
	{
		USpringArmComponent* SpringArm = MageCharacter->GetSpringArm();

		if (SpringArm)
		{
			SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength += ZoomAxis * 100.0f, 300.0f, 1200.0f);
		}
	}
}

void AMagePlayerController::CursorTrace()
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_Visibility, false, HitResult); //注意设置对应的碰撞通道
	if(!HitResult.bBlockingHit)
	{
		return;
	}

	
	LastActor = CurrentActor;
	CurrentActor = Cast<IEnemyInterface>(HitResult.GetActor());

	//光标射线追踪有几种情况：
	//代码逻辑可优化，这里为了便于理解没有优化
	if(LastActor == nullptr && CurrentActor == nullptr)
	{
		return;
	}
	else if(LastActor == nullptr && CurrentActor != nullptr)
	{
		CurrentActor->HighlightActor();
	}
	else if(LastActor != nullptr && CurrentActor == nullptr)
	{
		LastActor->UnHighlightActor();
	}
	else if(LastActor != nullptr && CurrentActor != nullptr && LastActor != CurrentActor)
	{
		LastActor->UnHighlightActor();
		CurrentActor->HighlightActor();
	}
	else if(LastActor != nullptr && CurrentActor != nullptr && LastActor == CurrentActor)
	{
		return;
	}
}
	

