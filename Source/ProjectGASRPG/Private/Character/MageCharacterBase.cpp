#include "ProjectGASRPG/Public/Character/MageCharacterBase.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageGameplayTags.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

AMageCharacterBase::AMageCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	
	/** Controller 旋转时不跟着旋转。让它只影响Camera。*/
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;

	/** 允许相机阻挡 */
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	Weapon->SetupAttachment(GetMesh(), TEXT("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMageCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

FVector AMageCharacterBase::GetWeaponSocketLocation_Implementation(const FGameplayTag& MontageTag) const
{
	const FMageGameplayTags GameplayTags = FMageGameplayTags::Get();
	for(auto Pair:AttackMontageTag_To_WeaponSocket)
	{
		if(MontageTag.MatchesTagExact(Pair.Key))
		{
			//如果武器没有指定Mesh，就用武器的Socket，否则用Mesh的Socket(比如手部)
			if(IsValid(Weapon->GetSkeletalMeshAsset()))
			{
				return Weapon->GetSocketLocation(Pair.Value);
			}
			return GetMesh()->GetSocketLocation(Pair.Value);
		}
	}
	return FVector::ZeroVector;
	
#pragma  region  不使用Map的方法，不太优雅
	// if(MontageTag.MatchesTagExact(GameplayTags.Montage_Attack_Weapon) && IsValid(Weapon))
	// {
	// 	return Weapon->GetSocketLocation(WeaponTipSocket);
	// }
	//
	// if(MontageTag.MatchesTagExact(GameplayTags.Montage_Attack_LeftHand))
	// {
	// 	return GetMesh()->GetSocketLocation(LeftHandSocket);
	// }
	//
	// if(MontageTag.MatchesTagExact(GameplayTags.Montage_Attack_RightHand))
	// {
	// 	return GetMesh()->GetSocketLocation(RightHandSocket);
	// }
#pragma endregion
}

void AMageCharacterBase::Dissolve()
{
	if(IsValid(DissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		if(IsValid(DynamicMaterialInstance))
		{
			//遍历所有overridematerial
			for(int32 i = 0; i < GetMesh()->GetNumMaterials(); i++)
			{
				GetMesh()->SetMaterial(i, DynamicMaterialInstance);
			}
			
			StartMeshDissolveTimeline(DynamicMaterialInstance);
		}
	}
	
	if(IsValid(WeaponMaterialInstance) && IsValid(Weapon->GetSkeletalMeshAsset()))
	{
		UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(WeaponMaterialInstance, this);
		if(IsValid(DynamicMaterialInstance))
		{
			for(int32 i = 0; i < GetMesh()->GetNumMaterials(); i++)
			{
				Weapon->SetMaterial(i, DynamicMaterialInstance);
			}
			
			StartWeaponDissolveTimeline(DynamicMaterialInstance);
		}
	}
}

UAnimMontage* AMageCharacterBase::GetHitReactMontage_Implementation() const
{
	return HitReactMontage;
}

void AMageCharacterBase::Die()
{
	Weapon->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	MulticastHandleDeath();
}

void AMageCharacterBase::MulticastHandleDeath_Implementation()
{
	
	/** 物理死亡效果（Ragdoll布娃娃） */
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/** 溶解 */
	Dissolve();
	
	bIsDead = true;
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

void AMageCharacterBase::InitDefaultAttributes() const
{
	//...
}

void AMageCharacterBase::AddCharacterAbilities() const
{
	if(!HasAuthority()) return;

	if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		MageASC->AddCharacterAbilities(CharacterAbilities);
	}
}

