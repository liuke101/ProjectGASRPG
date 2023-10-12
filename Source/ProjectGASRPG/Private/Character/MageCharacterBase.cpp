#include "ProjectGASRPG/Public/Character/MageCharacterBase.h"

#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AMageCharacterBase::AMageCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Controller 旋转时不跟着旋转。让它只影响Camera。
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; // 朝向旋转到移动方向，开启：后退转向，关闭：后退不转向
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponHandSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMageCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AMageCharacterBase::InitAbilityActorInfo()
{
	//...
}

void AMageCharacterBase::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	if(UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		checkf(GameplayEffectClass, TEXT("%s为空，请在角色蓝图中设置"), *GameplayEffectClass->GetName());
		const FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
		const FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);
		const FActiveGameplayEffectHandle ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

void AMageCharacterBase::InitDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttribute, 1.0f);
	ApplyEffectToSelf(DefaultSecondaryAttribute, 1.0f); 
}

