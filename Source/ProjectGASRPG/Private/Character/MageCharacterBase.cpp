#include "ProjectGASRPG/Public/Character/MageCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Component/DebuffNiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "Kismet/GameplayStatics.h"
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
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/** 存储默认最大速度 */
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	/* Debuff Niagara */
	BurnDebuffNiagara = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("BurnDebuffNiagaraComponent"));
	BurnDebuffNiagara->SetupAttachment(GetMesh());
	BurnDebuffNiagara->DebuffTag = FMageGameplayTags::Instance().Debuff_Type_Burn;

	FrozenDebuffNiagara = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("FrozenDebuffNiagaraComponent"));
	FrozenDebuffNiagara->SetupAttachment(GetMesh());
	FrozenDebuffNiagara->DebuffTag = FMageGameplayTags::Instance().Debuff_Type_Frozen;

	StunDebuffNiagara = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("StunDebuffNiagaraComponent"));
	StunDebuffNiagara->SetupAttachment(GetMesh());
	StunDebuffNiagara->DebuffTag = FMageGameplayTags::Instance().Debuff_Type_Stun;

	BleedDebuffNiagara = CreateDefaultSubobject<UDebuffNiagaraComponent>(TEXT("BleedDebuffNiagaraComponent"));
	BleedDebuffNiagara->SetupAttachment(GetMesh());
	BleedDebuffNiagara->DebuffTag = FMageGameplayTags::Instance().Debuff_Type_Bleed;
}

void AMageCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	CollectMeshComponents();
	//GetTracePointsLocation();
}

void AMageCharacterBase::GetTracePointsLocation()
{
	TracePointsLocation.Empty();
	for(const FName& SocketName: WeaponSocketNames)
	{
		TracePointsLocation.Add(Weapon->GetSocketLocation(SocketName));
	}
}

void AMageCharacterBase::AttackMontageWindowBegin()
{
	GetTracePointsLocation();
	
	//每0.1秒检测一次
	GetWorld()->GetTimerManager().SetTimer(AttackMontageWindowBegin_TimerHandle, this, &AMageCharacterBase::AttackMontageWindowBegin_Delay, 0.1f, true);
}

void AMageCharacterBase::AttackMontageWindowEnd()
{
	GetWorld()->GetTimerManager().ClearTimer(AttackMontageWindowBegin_TimerHandle);
}

void AMageCharacterBase::AttackMontageWindowBegin_Delay()
{
	for(int i = 0;i<WeaponSocketNames.Num();i++)
	{
		// TracePointsLocation[i] 上一帧位置
		// Weapon->GetSocketLocation(WeaponSocketNames[i]) 当前位置
		TArray<FHitResult> HitResults;
		UKismetSystemLibrary::LineTraceMulti(this, TracePointsLocation[i], Weapon->GetSocketLocation(WeaponSocketNames[i]),TraceTypeQuery1, false, {this}, EDrawDebugTrace::ForDuration, HitResults, true, FLinearColor::Red, FLinearColor::Green, 1.0f);

		for(auto HitResult: HitResults)
		{
			if(HitResult.bBlockingHit)
			{
				AActor* HitResultActor = HitResult.GetActor();
				//如果是敌人，就加入HitActors
				if(!UMageAbilitySystemLibrary::IsFriendly(this,HitResultActor))
				{
					HitActors.AddUnique(HitResultActor);
				}
			}
		}
	}

	//更新
	GetTracePointsLocation();
}

void AMageCharacterBase::AttackMontageWindowEnd_Delay()
{
}

FTaggedMontage AMageCharacterBase::GetRandomAttackMontage_Implementation() const
{
	if(TArray<FTaggedMontage> Montages= Execute_GetAttackMontages(this); !Montages.IsEmpty())
	{
		const int32 RandomIndex = FMath::RandRange(0, Montages.Num() - 1);
		return Montages[RandomIndex];
	}
	return FTaggedMontage();
}

FTaggedMontage AMageCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag) const
{
	for(FTaggedMontage TaggedMontage: AttackMontages)
	{
		if(TaggedMontage.MontageTag == MontageTag)
		{
			return TaggedMontage;
		}
	}
	return FTaggedMontage();
}


void AMageCharacterBase::StunTagChanged(const FGameplayTag CallbackTag, const int32 NewCount)
{
	bIsStun = NewCount > 0;
	
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
	FGameplayTagContainer BlockTags;
	BlockTags.AddTag(MageGameplayTags.Player_Block_InputPressed);
	BlockTags.AddTag(MageGameplayTags.Player_Block_InputReleased);
	BlockTags.AddTag(MageGameplayTags.Player_Block_InputHold);

	if(bIsStun)
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.0f; //禁止移动
		GetAbilitySystemComponent()->AddLooseGameplayTags(BlockTags);
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
		GetAbilitySystemComponent()->RemoveLooseGameplayTags(BlockTags);
	}
}

void AMageCharacterBase::FrozenTagChanged(const FGameplayTag CallbackTag, const int32 NewCount)
{
	if(NewCount>0)
	{
		CustomTimeDilation = 0.5f; //时间减速
		
	}
	else
	{
		CustomTimeDilation = 1.0f; //恢复
	}
}


FVector AMageCharacterBase::GetWeaponSocketLocationByTag_Implementation(const FGameplayTag& SocketTag) const
{
	const FMageGameplayTags GameplayTags = FMageGameplayTags::Instance();
	for(auto &Pair:AttackSocketTag_To_WeaponSocket)
	{
		FGameplayTag AttackSocketTag = Pair.Key;
		const FName AttackTriggerSocket = Pair.Value;
		
		if(SocketTag.MatchesTagExact(AttackSocketTag))
		{
			//如果武器指定了Mesh，就用武器的Socket，否则用Mesh的Socket(比如手部)
			if(IsValid(Weapon->GetSkeletalMeshAsset()))
			{
				return Weapon->GetSocketLocation(AttackTriggerSocket);
			}
			//TODO:暂不兼容ReratgetMesh
			return GetMesh()->GetSocketLocation(AttackTriggerSocket);
		}
	}
	return FVector::ZeroVector;
#pragma endregion
}

void AMageCharacterBase::Die(const FVector& DeathImpulse)
{
	/** 死亡音效 */
	UGameplayStatics::PlaySoundAtLocation(this,DeathSound,GetActorLocation(),GetActorRotation());
	
	/** 物理死亡效果（Ragdoll布娃娃） */
	/** 武器脱手 */
	Weapon->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	for(const auto MeshComponent: MeshComponents)
	{
		if(USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMeshComponent->SetSimulatePhysics(true);
			SkeletalMeshComponent->SetEnableGravity(true);
			SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
			SkeletalMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			SkeletalMeshComponent->AddImpulse(DeathImpulse,NAME_None,true); /** 死亡后加冲量击飞 */
		}
	}
	/** 溶解 */
	Dissolve();
	
	bIsDead = true;

	/** 死亡时停止Debuff */
	BurnDebuffNiagara->Deactivate(); 
	FrozenDebuffNiagara->Deactivate(); 
	StunDebuffNiagara->Deactivate(); 
	BleedDebuffNiagara->Deactivate(); 

	/** 广播死亡委托 */
	OnDeathDelegate.Broadcast(this);
}

void AMageCharacterBase::InitASC()
{
	//...
}

void AMageCharacterBase::InitDefaultAttributes() const
{
	//...
}


void AMageCharacterBase::CollectMeshComponents()
{
	GetMesh()->GetChildrenComponents(true, MeshComponents);
	MeshComponents.Add(GetMesh());
}

void AMageCharacterBase::Dissolve()
{
	if(IsValid(DissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		if(IsValid(DynamicMaterialInstance))
		{
			for(const auto MeshComponent: MeshComponents)
			{
				if(USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
				{
					//遍历所有overridematerial
					for(int32 i = 0; i < SkeletalMeshComponent->GetNumMaterials(); i++)
					{
						SkeletalMeshComponent->SetMaterial(i, DynamicMaterialInstance);
					}
				}
			}
			StartMeshDissolveTimeline(DynamicMaterialInstance);
		}
	}
}
