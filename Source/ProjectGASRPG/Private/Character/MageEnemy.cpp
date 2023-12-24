#include "Character/MageEnemy.h"

#include "AI/MageAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "GAS/MageGameplayTags.h"
#include "ProjectGASRPG/ProjectGASRPG.h"
#include "UI/Widgets/MageUserWidget.h"


AMageEnemy::AMageEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	
	AbilitySystemComponent = CreateDefaultSubobject<UMageAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UMageAttributeSet>(TEXT("AttributeSet"));

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	HealthBar->SetupAttachment(RootComponent);

	TargetingReticle = CreateDefaultSubobject<UWidgetComponent>(TEXT("TargetingReticle"));
	TargetingReticle->SetupAttachment(RootComponent);
}

void AMageEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	InitASC();
	
	/** 授予技能 */
	UMageAbilitySystemLibrary::GiveCharacterAbilities(this,AbilitySystemComponent,CharacterClass);
	
	InitUI();

	/** 受击反馈, 当角色被GE授予 Tag 时触发 */
	AbilitySystemComponent->RegisterGameplayTagEvent(FMageGameplayTags::Instance().Effects_HitReact,EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMageEnemy::HitReactTagChanged);
}

void AMageEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 
}

void AMageEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AMageEnemy::HighlightActor()
{
	for(const auto MeshComponent: MeshComponents)
	{
		if(USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMeshComponent->SetCustomDepthStencilValue(HighlightEnemyStencilMaskValue);
		}
	}
}

void AMageEnemy::UnHighlightActor()
{
	for(const auto MeshComponent: MeshComponents)
	{
		if(USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMeshComponent->SetCustomDepthStencilValue(DefaultEnemyStencilMaskValue);
		}
	}
}

void AMageEnemy::Die(const FVector& DeathImpulse)
{
	SetLifeSpan(LifeSpan);
	
	if(IsValid(MageAIController))
	{
		MageAIController->GetBlackboardComponent()->SetValueAsBool(FName("bDead"), true);
	}

	if(IsValid(HealthBar))
	{
		HealthBar->DestroyComponent();
	}

	if(IsValid(HUDHealthBar))
	{
		HUDHealthBar->RemoveFromParent();
	}

	if(IsValid(TargetingReticle))
	{
		TargetingReticle->DestroyComponent();
	}

	Super::Die(DeathImpulse);
}

void AMageEnemy::InitASC()
{
	if(AbilitySystemComponent == nullptr) return;
		
	/** 初始化ASC */
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	OnASCRegisteredDelegate.Broadcast(AbilitySystemComponent);

	/* 监听Debuff_Type_Stun变化, 回调设置眩晕状态 */
	AbilitySystemComponent->RegisterGameplayTagEvent(FMageGameplayTags::Instance().Debuff_Type_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMageEnemy::StunTagChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(FMageGameplayTags::Instance().Debuff_Type_Frozen, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMageEnemy::FrozenTagChanged);
	
	/** 绑定回调 */
	Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->BindEffectAppliedCallback();

	/** 初始化默认属性 */
	InitDefaultAttributes();
}

void AMageEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	MageAIController = Cast<AMageAIController>(NewController);
	if(IsValid(MageAIController))
	{
		MageAIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
		MageAIController->RunBehaviorTree(BehaviorTree);

		/** 初始化黑板键值对,FName对应在黑板中创建的Key */
		MageAIController->GetBlackboardComponent()->SetValueAsBool(FName("bHitReacting"), false);
		MageAIController->GetBlackboardComponent()->SetValueAsBool(FName("bRangeAttacker"), CharacterClass != ECharacterClass::Warrior);
	}
	
}


void AMageEnemy::InitDefaultAttributes() const
{
	UMageAbilitySystemLibrary::InitDefaultAttributesByCharacterClass(this, CharacterClass, GetCharacterLevel(), AbilitySystemComponent);
}

void AMageEnemy::HitReactTagChanged(const FGameplayTag CallbackTag, const int32 NewCount)
{
	bHitReacting = NewCount > 0;
	
	GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.0f : DefaultMaxWalkSpeed;

	MageAIController->GetBlackboardComponent()->SetValueAsBool(FName("bHitReacting"), bHitReacting);
}

void AMageEnemy::StunTagChanged(const FGameplayTag CallbackTag, const int32 NewCount)
{
	Super::StunTagChanged(CallbackTag, NewCount);

	MageAIController->GetBlackboardComponent()->SetValueAsBool(FName("bIsStun"), bIsStun);
}


void AMageEnemy::InitUI()
{
	/** Enemy本身作为 WidgetController */
	HUDHealthBar = CreateWidget<UMageUserWidget>(GetWorld(), HUDHealthBarClass);
	if(HUDHealthBar)
	{
		HUDHealthBar->SetWidgetController(this);
		HUDHealthBar->AddToViewport();
		HUDHealthBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if(UMageUserWidget* HealthBarWidget =  Cast<UMageUserWidget>(HealthBar->GetUserWidgetObject()))
	{
		HealthBarWidget->SetWidgetController(this);
	}
	if(UMageUserWidget* TargetingReticleWidget =  Cast<UMageUserWidget>(TargetingReticle->GetUserWidgetObject()))
	{
		TargetingReticleWidget->SetWidgetController(this);
		TargetingReticleWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

