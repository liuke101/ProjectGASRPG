#include "Character/MageCharacter.h"

#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/Data/LevelDataAsset.h"
#include "Player/MagePlayerController.h"
#include "Player/MagePlayerState.h"
#include "UI/HUD/MageHUD.h"


AMageCharacter::AMageCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	GetCharacterMovement()->bUseControllerDesiredRotation = false; //使用控制器的旋转, 人物始终跟随镜头转向
	GetCharacterMovement()->bOrientRotationToMovement = true; //朝向旋转到移动方向，开启：后退转向，关闭：后退不转向
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));
	SpringArm->SetRelativeRotation(FRotator(-40.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = true;
	/* 镜头延迟平滑 */
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 20.0f;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;

	/** 升级特效 */
	LevelUpNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LevelUpNiagaraComponent"));
	LevelUpNiagara->SetupAttachment(RootComponent);
	LevelUpNiagara->bAutoActivate = false;
	
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


void AMageCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
	GiveCharacterAbilities();
}

void AMageCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	InitAbilityActorInfo();
}


void AMageCharacter::InitDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttribute, GetCharacterLevel());
	ApplyEffectToSelf(DefaultVitalAttribute, GetCharacterLevel()); //VitalAttribute基于SecondaryAttribute生成初始值，所以先让SecondaryAttribute初始化
	ApplyEffectToSelf(DefaultResistanceAttribute, GetCharacterLevel());
}

void AMageCharacter::GiveCharacterAbilities() const
{
	if(!HasAuthority()) return;

	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		MageASC->GiveCharacterAbilities(CharacterAbilities);
		MageASC->GivePassiveAbilities(PassiveAbilities);
	}
}

void AMageCharacter::InitAbilityActorInfo()
{
	/* 该函数被PossessedBy() 和 OnRep_PlayerState()调用 */
	AMagePlayerState* MagePlayerState = GetPlayerState<AMagePlayerState>();
	if(MagePlayerState == nullptr) return;

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

	/* 初始化默认属性 */
	InitDefaultAttributes();
	
}

AMagePlayerState* AMageCharacter::GetMagePlayerState() const
{
	AMagePlayerState* MagePlayerState = GetPlayerState<AMagePlayerState>();
	checkf(MagePlayerState, TEXT("MagePlayerState未设置"));
	return MagePlayerState;
}

int32 AMageCharacter::GetExp() const
{
	return GetMagePlayerState()->GetExp();
}

void AMageCharacter::AddToExp(const int32 InExp)
{
	GetMagePlayerState()->AddToExp(InExp);
}

void AMageCharacter::LevelUp()
{
	MulticastLevelUpEffect();
}

int32 AMageCharacter::FindLevelForExp(const int32 InExp) const
{
	return GetMagePlayerState()->LevelDataAsset->FindLevelForExp(InExp);
}

void AMageCharacter::AddToLevel(const int32 InLevel)
{
	GetMagePlayerState()->AddToLevel(InLevel);
}

int32 AMageCharacter::GetAttributePointReward(const int32 Level) const
{
	return GetMagePlayerState()->LevelDataAsset->LevelUpInfos[Level].AttributePointReward;
}

void AMageCharacter::AddToAttributePoint(const int32 InPoints)
{
	GetMagePlayerState()->AddToAttributePoint(InPoints);
}

int32 AMageCharacter::GetAttributePoint() const
{
	return GetMagePlayerState()->GetAttributePoint();
}

int32 AMageCharacter::GetSkillPointReward(const int32 Level) const
{
	return GetMagePlayerState()->LevelDataAsset->LevelUpInfos[Level].SkillPointReward;
}

void AMageCharacter::AddToSkillPoint(const int32 InPoints)
{
	GetMagePlayerState()->AddToSkillPoint(InPoints);
}

int32 AMageCharacter::GetSkillPoint() const
{
	return GetMagePlayerState()->GetSkillPoint();
}


void AMageCharacter::MulticastLevelUpEffect_Implementation() const
{
	if(IsValid(LevelUpNiagara))
	{
		const FVector CameraLocation = FollowCamera->GetComponentLocation();
		const FVector NiagaraLocation = LevelUpNiagara->GetComponentLocation();
		const FRotator ToCameraRotation = (CameraLocation-NiagaraLocation).Rotation();
		LevelUpNiagara->SetWorldRotation(ToCameraRotation); //设置特效朝向镜头
		LevelUpNiagara->Activate(true);
	}
}

int32 AMageCharacter::GetCharacterLevel() const
{
	return GetMagePlayerState()->GetCharacterLevel();
}

ECharacterClass AMageCharacter::GetCharacterClass() const
{
	return GetMagePlayerState()->GetCharacterClass();
}
