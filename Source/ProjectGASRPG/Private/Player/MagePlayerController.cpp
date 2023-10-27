#include "Player/MagePlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Character/MageCharacter.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"
#include "Input/MageInputComponent.h"
#include "Interface/EnemyInterface.h"
#include "UI/Widgets/DamageFloatingTextComponent.h"

AMagePlayerController::AMagePlayerController()
{
	bReplicates = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
}

void AMagePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
	AutoRun();
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

	/** 绑定 InputAction */
	if (UMageInputComponent* MageInputComponent = CastChecked<UMageInputComponent>(InputComponent)) //自定义的增强输入组件
	{
		// Move: WASD
		if (MoveAction)
		{
			MageInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMagePlayerController::Move);
		}

		// Look: 【经典MMO模式】按住鼠标右键旋转视角
		if (LookAction)
		{
			MageInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMagePlayerController::Look);
		}

		// LookAround: 【动作模式】按住Alt+鼠标右键旋转视角 【经典MMO模式】Alt还原视角
		if (LookAroundAction)
		{
			MageInputComponent->BindAction(LookAroundAction, ETriggerEvent::Started, this,
			                               &AMagePlayerController::LookAroundStart);
			MageInputComponent->BindAction(LookAroundAction, ETriggerEvent::Completed, this,
			                               &AMagePlayerController::LookAroundEnd);
		}

		// CameraZoom: 鼠标滚轮缩放视野
		if (CameraZoomAction)
		{
			MageInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this,
			                               &AMagePlayerController::CameraZoom);
		}

		if (CtrlAction)
		{
			MageInputComponent->BindAction(CtrlAction, ETriggerEvent::Started, this,
			                               &AMagePlayerController::CtrlPressed);
			MageInputComponent->BindAction(CtrlAction, ETriggerEvent::Completed, this,
			                               &AMagePlayerController::CtrlReleased);
		}

		// AbilityInputActions
		if (MageInputConfig)
		{
			MageInputComponent->BindAbilityInputActions(MageInputConfig, this,
			                                            &AMagePlayerController::AbilityInputTagPressed,
			                                            &AMagePlayerController::AbilityInputTagHold,
			                                            &AMagePlayerController::AbilityInputTagReleased);
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
	FVector2D LookAxisVector = InputActionValue.Get<FVector2D>(); //X轴对应左右（Yaw），Y轴对应上下（Pitch）

	// add yaw and pitch input to controller
	GetPawn()->AddControllerYawInput(LookAxisVector.X);
	GetPawn()->AddControllerPitchInput(LookAxisVector.Y);
}

void AMagePlayerController::LookAroundStart()
{
	GetCharacter()->GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void AMagePlayerController::LookAroundEnd()
{
	/* 视角还原 */
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	if (AMageCharacter* MageCharacter = Cast<AMageCharacter>(GetCharacter()))
	{
		FRotator SpringArmRotation = MageCharacter->GetSpringArm()->GetComponentRotation();
		FRotator CurrentControlRotation = PlayerController->GetControlRotation();
		//将SpringArm的位置和朝向匹配到当前Character的朝向
		PlayerController->SetControlRotation(FRotator(CurrentControlRotation.Pitch, SpringArmRotation.Yaw,
		                                              CurrentControlRotation.Roll));
	}

	GetCharacter()->GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void AMagePlayerController::CameraZoom(const FInputActionValue& InputActionValue)
{
	// input is a Vector2D
	float ZoomAxis = InputActionValue.Get<float>();

	if (AMageCharacter* MageCharacter = Cast<AMageCharacter>(GetCharacter()))
	{
		USpringArmComponent* SpringArm = MageCharacter->GetSpringArm();

		if (SpringArm)
		{
			SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength += ZoomAxis * 100.0f, 300.0f, 1200.0f);
		}
	}
}

void AMagePlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	/* 鼠标左键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Get().Input_LMB))
	{
		if (!bTargeting())
		{
			const APawn* ControlPawn = GetPawn();
			if (ControlPawn && FollowTime <= ShortPressThreshold)
			{
				bAutoRunning = true;
				/**
				 * 根据 NavMesh 上的 PathPoints 创建样条线路径点
				 *
				 * 客户端默认关闭Navigation System，是的客户端无法寻路，进行如下设置：
				 * 项目设置->导航系统->勾选运行客户端导航
				 */

				SetCachedDestinationFromCursorHit();

				if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(
					this, ControlPawn->GetActorLocation(), CachedDestination, nullptr))
				{
					SplineComponent->ClearSplinePoints();
					for (auto PointLocation : NavPath->PathPoints)
					{
						SplineComponent->AddSplinePoint(PointLocation, ESplineCoordinateSpace::World);
						DrawDebugSphere(GetWorld(), PointLocation, 8.0f, 12, FColor::Green, false, 5.0f);
					}
					CachedDestination = NavPath->PathPoints.Last();
				}
			}
		}
	}

	/* 1键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Get().Input_1))
	{
		// if (bTargeting()) //若鼠标选中了物体，且有AbilitySystemComponent, 则激活技能
		// {
			if (GetAbilitySystemComponent())
			{
				GetAbilitySystemComponent()->AbilityInputTagHold(InputTag);
			}
		// }
	}
}

void AMagePlayerController::AbilityInputTagHold(FGameplayTag InputTag)

{
	/* 鼠标左键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Get().Input_LMB))
	{
		//长按时，若鼠标没有选中物体，则进行移动
		if (!bTargeting())
		{
			FollowTime += GetWorld()->GetDeltaSeconds();
			if (FollowTime > ShortPressThreshold)
			{
				bAutoRunning = false;

				SetCachedDestinationFromCursorHit();

				if (APawn* ControlPawn = GetPawn())
				{
					const FVector WorldDirection = (CachedDestination - ControlPawn->GetActorLocation()).
						GetSafeNormal();
					ControlPawn->AddMovementInput(WorldDirection);
				}
			}
		}
	}
}

void AMagePlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	/* 鼠标左键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Get().Input_LMB))
	{
		FollowTime = 0.0f;
	}
}

UMageAbilitySystemComponent* AMagePlayerController::GetAbilitySystemComponent()
{
	if (MageAbilitySystemComponent == nullptr)
	{
		MageAbilitySystemComponent = Cast<UMageAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));
	}
	return MageAbilitySystemComponent;
}

void AMagePlayerController::CursorTrace()
{
	GetHitResultUnderCursor(ECC_Visibility, false, CursorHitResult); //注意设置对应的碰撞通道

	if (!CursorHitResult.bBlockingHit) return;

	LastActor = CurrentActor;
	CurrentActor = Cast<IEnemyInterface>(CursorHitResult.GetActor());

	// 光线射线追踪，物体高亮。
	if (LastActor != CurrentActor)
	{
		if (LastActor) LastActor->UnHighlightActor();
		if (CurrentActor) CurrentActor->HighlightActor();
	}

#pragma region 光标射线追踪的情况（未优化代码）
	// if (LastActor == nullptr && CurrentActor == nullptr)
	// {
	// 	return;
	// }
	// else if (LastActor == nullptr && CurrentActor != nullptr)
	// {
	// 	CurrentActor->HighlightActor();
	// }
	// else if (LastActor != nullptr && CurrentActor == nullptr)
	// {
	// 	LastActor->UnHighlightActor();
	// }
	// else if (LastActor != nullptr && CurrentActor != nullptr && LastActor != CurrentActor)
	// {
	// 	LastActor->UnHighlightActor();
	// 	CurrentActor->HighlightActor();
	// }
	// else if (LastActor != nullptr && CurrentActor != nullptr && LastActor == CurrentActor)
	// {
	// 	return;
	// }
#pragma endregion
}

void AMagePlayerController::AutoRun()
{
	if (!bAutoRunning) return;

	if (APawn* ControlPawn = GetPawn())
	{
		const FVector LocationOnSpline = SplineComponent->FindLocationClosestToWorldLocation(
			ControlPawn->GetActorLocation(),
			ESplineCoordinateSpace::World);
		const FVector Direction = SplineComponent->FindDirectionClosestToWorldLocation(LocationOnSpline,
			ESplineCoordinateSpace::World);
		ControlPawn->AddMovementInput(Direction);

		const float DistanceToDestination = FVector::Dist(ControlPawn->GetActorLocation(), CachedDestination);
		if (DistanceToDestination <= AutoRunAcceptanceRadius)
		{
			bAutoRunning = false;
		}
	}
}

bool AMagePlayerController::bTargeting()
{
	return CurrentActor ? true : false;
}

void AMagePlayerController::SetCachedDestinationFromCursorHit()
{
	if (CursorHitResult.bBlockingHit)
	{
		CachedDestination = CursorHitResult.ImpactPoint;
	}
}


void AMagePlayerController::AttachDamageFloatingTextToTarget_Implementation(float DamageValue, ACharacter* TargetCharacter, bool bIsCriticalHit)
{
	if(IsValid(TargetCharacter) && IsLocalController())
	{
		checkf(DamageFloatingTextComponentClass, TEXT("DamageFloatingTextComponentClass 为空,请在 BP_MagePlayerController 中设置"));
		UDamageFloatingTextComponent* DamageFloatingTextComponent = NewObject<UDamageFloatingTextComponent>(TargetCharacter,
			DamageFloatingTextComponentClass);
		DamageFloatingTextComponent->RegisterComponent();
		DamageFloatingTextComponent->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DamageFloatingTextComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		
		DamageFloatingTextComponent->SetDamageFloatingText(DamageValue, bIsCriticalHit);
		
	}
}
