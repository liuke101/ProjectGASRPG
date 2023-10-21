#include "Character/MageEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAttributeSet.h"
#include "ProjectGASRPG/ProjectGASRPG.h"
#include "UI/Widgets/MageUserWidget.h"


AMageEnemy::AMageEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
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
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(CustomDepthStencilValue);
	Weapon->SetRenderCustomDepth(true);
	Weapon->SetCustomDepthStencilValue(CustomDepthStencilValue);
}

void AMageEnemy::UnHighlightActor()
{
	GetMesh()->SetRenderCustomDepth(false);
	Weapon->SetRenderCustomDepth(false);
}

void AMageEnemy::InitAbilityActorInfo()
{
	// 初始化ASC
	if(AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	Cast<UMageAbilitySystemComponent>(AbilitySystemComponent)->BindEffectCallbacks();

	InitDefaultAttributes();
}

