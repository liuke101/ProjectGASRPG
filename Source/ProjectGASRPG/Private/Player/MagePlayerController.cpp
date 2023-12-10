#include "Player/MagePlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/MageCharacter.h"
#include "Components/SplineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "Input/MageInputComponent.h"
#include "Interface/InteractionInterface.h"
#include "ProjectGASRPG/ProjectGASRPG.h"
#include "UI/Widgets/DamageFloatingTextComponent.h"

AMagePlayerController::AMagePlayerController()
{
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
}

void AMagePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	//CursorTrace();
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
	
	//将玩家自身添加到忽略列表中
	TargetingIgnoreActors.Add(GetPawn());
	SwitchTargetCount = MaxSwitchTargetCount;
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

		// 绑定 AbilityInputActions
		if (InputConfigDataAsset)
		{
			MageInputComponent->BindAbilityInputActions(InputConfigDataAsset, this,
			                                            &AMagePlayerController::AbilityInputTagStarted,
			                                            &AMagePlayerController::AbilityInputTagOngoing,
			                                            &AMagePlayerController::AbilityInputTagTriggered,
			                                            &AMagePlayerController::AbilityInputTagCanceled,
			                                            &AMagePlayerController::AbilityInputTagCompleted);
		}
	}
}

void AMagePlayerController::Move(const FInputActionValue& InputActionValue)
{
	if(GetMageASC() && GetMageASC()->HasMatchingGameplayTag(FMageGameplayTags::Instance().Player_Block_InputPressed))
	{
		return;
	}
	
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

void AMagePlayerController::AbilityInputTagStarted(FGameplayTag InputTag)
{
	if(GetMageASC() && GetMageASC()->HasMatchingGameplayTag(FMageGameplayTags::Instance().Player_Block_InputPressed))
	{
		return;
	}
	/** 不同键位的特有操作，使用if else if 连接。技能逻辑单独在GA中设置 */
	
	/* 鼠标左键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Instance().Input_LMB))
	{
		CursorHitTargeting();
		
		if (!HasTargetingActor())
		{
			if (GetPawn() && FollowTime <= ShortPressThreshold)
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
					this, GetPawn()->GetActorLocation(), CachedDestination, nullptr))
				{
					SplineComponent->ClearSplinePoints();
					for (auto PointLocation : NavPath->PathPoints)
					{
						SplineComponent->AddSplinePoint(PointLocation, ESplineCoordinateSpace::World);
						//DrawDebugSphere(GetWorld(), PointLocation, 8.0f, 12, FColor::Green, false, 5.0f);
					}
					
					if(!NavPath->PathPoints.IsEmpty())
					{
						CachedDestination = NavPath->PathPoints.Last();
					}
				}

				//Niagara点击特效
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,ClickNiagaraSystem, CachedDestination);
			}
		}
	}
	/** Tab 切换目标 */
	else if (InputTag.MatchesTagExact(FMageGameplayTags::Instance().Input_Tab))
	{
		SwitchCombatTarget();
	}
	

	/** 上面通过匹配按键执行特有的操作，这里激活所有匹配的技能，逻辑在GA中设置 */	
	GetMageASC()->AbilityInputTagStarted(InputTag);
}

void AMagePlayerController::AbilityInputTagOngoing(FGameplayTag InputTag)
{
}

void AMagePlayerController::AbilityInputTagTriggered(FGameplayTag InputTag)

{
	if(GetMageASC()->HasMatchingGameplayTag(FMageGameplayTags::Instance().Player_Block_InputHold))
	{
		return;
	}
	
	/* 鼠标左键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Instance().Input_LMB))
	{
		CursorHitTargeting();
		
		//长按时，若鼠标没有选中物体，则进行移动
		if (!HasTargetingActor())
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
	
	GetMageASC()->AbilityInputTagTriggered(InputTag);
}

void AMagePlayerController::AbilityInputTagCanceled(FGameplayTag InputTag)
{
	
}

void AMagePlayerController::AbilityInputTagCompleted(FGameplayTag InputTag)
{
	if(GetMageASC()->HasMatchingGameplayTag(FMageGameplayTags::Instance().Player_Block_InputReleased))
	{
		return;
	}
	
	/* 鼠标左键 */
	if (InputTag.MatchesTagExact(FMageGameplayTags::Instance().Input_LMB))
	{
		FollowTime = 0.0f;
	}
	
	
	GetMageASC()->AbilityInputTagCompleted(InputTag);
}

UMageAbilitySystemComponent* AMagePlayerController::GetMageASC()
{
	if (MageAbilitySystemComponent == nullptr)
	{
		MageAbilitySystemComponent = Cast<UMageAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()));
	}
	checkf(MageAbilitySystemComponent, TEXT("MageAbilitySystemComponent 为空"));
	return MageAbilitySystemComponent;
}

void AMagePlayerController::CursorHitTargeting()
{
	if(GetMageASC()->HasMatchingGameplayTag(FMageGameplayTags::Instance().Player_Block_CursorTrace))
	{
		CancelTargetingActor();
		return;
	}
	
	GetHitResultUnderCursor(ECC_Target, false, CursorHitResult); //注意设置对应的碰撞通道

	if (CursorHitResult.bBlockingHit)
	{
		//只有实现了UInteractionInterface的Actor才能被选中
		if(CursorHitResult.GetActor()->Implements<UInteractionInterface>())
		{
			SwitchTargetingActor(CursorHitResult.GetActor());
		}
	}
	
#pragma endregion
}

void AMagePlayerController::SwitchTargetingActor(AActor* NewTargetActor)
{
	LastTargetingActor = CurrentTargetingActor;
	CurrentTargetingActor = NewTargetActor;

	//切换高亮
	if (LastTargetingActor != CurrentTargetingActor)
	{
		if(IInteractionInterface* InteractionInterface = Cast<IInteractionInterface>(LastTargetingActor))
		{
			InteractionInterface->UnHighlightActor();
		}
		if(IInteractionInterface* InteractionInterface = Cast<IInteractionInterface>(CurrentTargetingActor))
		{
			InteractionInterface->HighlightActor();
		}
	}
	
	// TODO：玩家进入休战状态，几秒后取消选中
	// if(TargetingTimerHandle.IsValid())
	// {
	// 	GetWorldTimerManager().ClearTimer(TargetingTimerHandle);
	// }
	// GetWorldTimerManager().SetTimer(TargetingTimerHandle, this, &AMagePlayerController::CancelTargetingActor, TargetingTime, false);
}

void AMagePlayerController::SwitchCombatTarget()
{
	//清空缓存
	TargetingActors.Empty();
	if(TargetingIgnoreActors.Num() == SwitchTargetCount + 1) //+1是因为TargetingIgnoreActors中包含了玩家自身
	{
		TargetingIgnoreActors.Empty();
		TargetingIgnoreActors.Add(GetPawn());
	}

	//获取碰撞体内活着的敌人
	UMageAbilitySystemLibrary::GetLivingActorInCollisionShape(this, TargetingActors, TargetingIgnoreActors, GetPawn()->GetActorLocation(), EColliderShape::Sphere,2000.0f);

	if(TargetingActors.Num() < MaxSwitchTargetCount)
	{
		SwitchTargetCount = TargetingActors.Num();
	}
	else
	{
		SwitchTargetCount = MaxSwitchTargetCount;
	}
	
	//获取最近的目标
	if(AActor* ClosestActor = UMageAbilitySystemLibrary::GetClosestActor(TargetingActors, GetPawn()->GetActorLocation()))
	{
		TargetingIgnoreActors.Add(ClosestActor);
	
		SwitchTargetingActor(ClosestActor);
	
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("ClosestActor: %s"), *ClosestActor->GetName()));
	}
}

void AMagePlayerController::CancelTargetingActor()
{
	if(IInteractionInterface* InteractionInterface = Cast<IInteractionInterface>(LastTargetingActor))
	{
		InteractionInterface->UnHighlightActor();
	}
	if(IInteractionInterface* InteractionInterface = Cast<IInteractionInterface>(CurrentTargetingActor))
	{
		InteractionInterface->UnHighlightActor();
	}
	LastTargetingActor = CurrentTargetingActor = nullptr;
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


void AMagePlayerController::SetCachedDestinationFromCursorHit()
{
	if (CursorHitResult.bBlockingHit)
	{
		CachedDestination = CursorHitResult.ImpactPoint;
	}
}


void AMagePlayerController::AttachDamageFloatingTextToTarget(float DamageValue, ACharacter* TargetCharacter, bool bIsCriticalHit)
{
	if(IsValid(TargetCharacter))
	{
		checkf(DamageFloatingTextComponentClass, TEXT("DamageFloatingTextComponentClass 为空,请在 BP_MagePlayerController 中设置"));
		UDamageFloatingTextComponent* DamageFloatingTextComponent = NewObject<UDamageFloatingTextComponent>(TargetCharacter, DamageFloatingTextComponentClass);
		DamageFloatingTextComponent->RegisterComponent();
		DamageFloatingTextComponent->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		DamageFloatingTextComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		
		DamageFloatingTextComponent->SetDamageFloatingText(DamageValue, bIsCriticalHit);
		
	}
}
