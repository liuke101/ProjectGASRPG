#include "Player/MagePlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/MageCharacter.h"
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
	// 注：InputComponent在项目设置->Engine->Input中设置
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMagePlayerController::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMagePlayerController::Look);
		EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &AMagePlayerController::CameraZoom);
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

void AMagePlayerController::CameraZoom(const FInputActionValue& InputActionValue)
{
	// input is a Vector2D
	float ZoomAxis = InputActionValue.Get<float>(); 
	
	if(AMageCharacter* MageCharacter = Cast<AMageCharacter>(GetPawn()))
	{
		MageCharacter->SetCameraDistance(ZoomAxis);
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
	

