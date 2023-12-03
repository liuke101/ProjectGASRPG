#include "Character/MageCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilityBlueprint.h"
#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"
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

AMagePlayerState* AMageCharacter::GetMagePlayerState() const
{
	AMagePlayerState* MagePlayerState = GetPlayerState<AMagePlayerState>();
	checkf(MagePlayerState, TEXT("MagePlayerState未设置"));
	return MagePlayerState;
}

void AMageCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitASC();
	GiveCharacterAbilities();
}

void AMageCharacter::InitDefaultAttributes() const
{
	for(auto AttributeEffect:DefaultAttributeEffects)
	{
		UMageAbilitySystemLibrary::ApplyEffectToSelf(GetAbilitySystemComponent(), AttributeEffect, GetCharacterLevel());
	}
}

void AMageCharacter::GiveCharacterAbilities() const
{
	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		MageASC->GiveCharacterAbilities(CharacterAbilities);
		MageASC->GivePassiveAbilities(PassiveAbilities);
	}
}

void AMageCharacter::InitASC()
{
	/*
	 * 该函数被 PossessedBy()调用 
	 *
	 * PossessedBy(): 在服务器上设置 ASC
	 * - AI 没有 PlayerController，因此我们可以在这里再次 init 以确保万无一失。
	 * - 对于拥有 PlayerController 的 Character，init两次也无妨。
	 */

	/** 注意角色类的ASC就是PlayerState中创建的ASC */
	AMagePlayerState* MagePlayerState = GetMagePlayerState();
	AbilitySystemComponent = MagePlayerState->GetAbilitySystemComponent();
	
	/* 初始化 AbilityActorInfo, 其中保存了 ASC 的 AvatarActor 和 OwnerActor */
	AbilitySystemComponent->InitAbilityActorInfo(MagePlayerState, this);

	/** 绑定回调 */
	Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->BindEffectAppliedCallback();
	
	OnASCRegisteredDelegate.Broadcast(AbilitySystemComponent);

	/* 监听Debuff_Type_Stun变化, 回调设置触电状态 */
	AbilitySystemComponent->RegisterGameplayTagEvent(FMageGameplayTags::Instance().Debuff_Type_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMageCharacter::StunTagChanged);
	
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
	if(IsValid(LevelUpNiagara))
	{
		const FVector CameraLocation = FollowCamera->GetComponentLocation();
		const FVector NiagaraLocation = LevelUpNiagara->GetComponentLocation();
		const FRotator ToCameraRotation = (CameraLocation-NiagaraLocation).Rotation();
		LevelUpNiagara->SetWorldRotation(ToCameraRotation); //设置特效朝向镜头
		LevelUpNiagara->Activate(true);
	}
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

int32 AMageCharacter::GetCharacterLevel() const
{
	return GetMagePlayerState()->GetCharacterLevel();
}

ECharacterClass AMageCharacter::GetCharacterClass() const
{
	return GetMagePlayerState()->GetCharacterClass();
}
