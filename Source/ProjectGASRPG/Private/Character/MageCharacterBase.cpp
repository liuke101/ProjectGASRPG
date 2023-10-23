#include "ProjectGASRPG/Public/Character/MageCharacterBase.h"

#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

AMageCharacterBase::AMageCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	/** Controller 旋转时不跟着旋转。让它只影响Camera。*/
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; // 朝向旋转到移动方向，开启：后退转向，关闭：后退不转向

	/** 允许相机阻挡 */
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	Weapon->SetupAttachment(GetMesh(), TEXT("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMageCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

FVector AMageCharacterBase::GetWeaponSocketLocation()
{
	checkf(Weapon, TEXT("%s的Weapon为空，请在角色蓝图中设置"), *GetName());
	return Weapon->GetSocketLocation(WeaponTipSocketName);
}

UAnimMontage* AMageCharacterBase::GetHitReactMontage_Implementation() const
{
	return HitReactMontage;
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
		FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
		EffectContextHandle.AddSourceObject(this);
		const FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);
		const FActiveGameplayEffectHandle ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

int32 AMageCharacterBase::GetCharacterLevel() const
{
	return 0;
}

void AMageCharacterBase::InitDefaultAttributes() const
{
	UMageAbilitySystemLibrary::InitDefaultAttributes(this, CharacterClass, GetCharacterLevel(), AbilitySystemComponent);

#pragma region 旧代码
	// ApplyEffectToSelf(DefaultPrimaryAttribute,  GetCharacterLevel());
	// ApplyEffectToSelf(DefaultSecondaryAttribute,  GetCharacterLevel());
	// ApplyEffectToSelf(DefaultVitalAttribute,  GetCharacterLevel()); //VitalAttribute基于SecondaryAttribute生成初始值，所以先让SecondaryAttribute初始化
#pragma endregion
}

void AMageCharacterBase::AddCharacterAbilities() const
{
	if(!HasAuthority()) return;

	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		MageASC->AddCharacterAbilities(CharacterAbilities);
	}
}

