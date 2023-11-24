#include "Character/MageEnemy.h"

#include "AI/MageAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
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
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<UMageAttributeSet>(TEXT("AttributeSet"));

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	HealthBar->SetupAttachment(RootComponent);
}

void AMageEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilityActorInfo();

	/** 给角色授予技能 */
	if(HasAuthority())
	{
		UMageAbilitySystemLibrary::GiveCharacterAbilities(this,AbilitySystemComponent,CharacterClass);
	}
	
	if(UMageUserWidget* MageUserWidget =  Cast<UMageUserWidget>(HealthBar->GetUserWidgetObject()))
	{
		/** Enemy本身作为WidgetController，绑定OnHealthChanged回调 */
		MageUserWidget->SetWidgetController(this);
	}
	
	if(const UMageAttributeSet* MageAttributeSet = Cast<UMageAttributeSet>(AttributeSet))
	{	/** 绑定ASC属性变化回调，接收属性变化 */
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		});
	
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MageAttributeSet->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		});

		/** WidgetController 广播初始值，委托回调在SetWidgetController(this)中绑定 */
		OnHealthChanged.Broadcast(MageAttributeSet->GetHealth());
		OnMaxHealthChanged.Broadcast(MageAttributeSet->GetMaxHealth());

		/** 受击反馈, 当角色被GE授予 Tag 时触发 */
		AbilitySystemComponent->RegisterGameplayTagEvent(FMageGameplayTags::Get().Effects_HitReact,EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AMageEnemy::HitReactTagChanged);
	}
	
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
			SkeletalMeshComponent->SetCustomDepthStencilValue(HighlightActorStencilMaskValue);
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
	
	Super::Die(DeathImpulse);
}

void AMageEnemy::InitAbilityActorInfo()
{
	if(AbilitySystemComponent == nullptr) return;
		
	/** 初始化ASC */
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	OnASCRegisteredDelegate.Broadcast(AbilitySystemComponent);
	
	/** 绑定回调 */
	Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->BindEffectCallbacks();

	/** 初始化默认属性 */
	if(HasAuthority())
	{
		InitDefaultAttributes();
	}
}

void AMageEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if(!HasAuthority()) return;
	
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
	UMageAbilitySystemLibrary::InitDefaultAttributes(this, CharacterClass, GetCharacterLevel(), AbilitySystemComponent);
}

void AMageEnemy::HitReactTagChanged(const FGameplayTag CallbackTag, const int32 NewCount)
{
	bHitReacting = NewCount > 0;
	
	GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.0f : GetCharacterMovement()->MaxWalkSpeed;

	/** AIController 仅服务器可用 */
	if(HasAuthority())
	{
		MageAIController->GetBlackboardComponent()->SetValueAsBool(FName("bHitReacting"), bHitReacting);
	}
}


