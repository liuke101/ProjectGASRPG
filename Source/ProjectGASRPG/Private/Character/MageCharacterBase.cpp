#include "ProjectGASRPG/Public/Character/MageCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
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
}

void AMageCharacterBase::GetAttackDetectionPointsLocation()
{
	AttackDetectionPointsLocation.Empty();
	AttackDetectionPointsLocation = GetAttackDetectionSocketLocationByTag_Implementation(MontageEventTag);
	checkf(!AttackDetectionPointsLocation.IsEmpty(), TEXT("%s的AttackDetectionSocketLocation为空，请在蓝图中设置"), *GetName());
}

void AMageCharacterBase::AttackMontageWindowBegin()
{
	GetAttackDetectionPointsLocation();
	
	//每x秒检测一次，时间间隔越短越平滑
	GetWorld()->GetTimerManager().SetTimer(AttackMontageWindowBegin_TimerHandle, this, &AMageCharacterBase::AttackMontageWindowBegin_Delay, 0.01f, true);
}

void AMageCharacterBase::AttackMontageWindowEnd()
{
	GetWorld()->GetTimerManager().ClearTimer(AttackMontageWindowBegin_TimerHandle);
	
	for(const auto HitActor: HitActors)
	{
		if(HitActor->Implements<UCombatInterface>())
		{
			Execute_SetIsDamageDetected(HitActor,false);
		}
	}
	HitActors.Empty();
}

void AMageCharacterBase::AttackMontageWindowBegin_Delay()
{
	for(int i = 0;i<AttackDetectionPointsLocation.Num();i++)
	{
		// TracePointsLocation[i] 上一帧位置
		// Weapon->GetSocketLocation(WeaponSocketNames[i]) 当前位置
		TArray<FHitResult> HitResults;
		//线条检测(适合精准检测)
		//UKismetSystemLibrary::LineTraceMulti(this, TracePointsLocation[i], Weapon->GetSocketLocation(WeaponSocketNames[i]),TraceTypeQuery1, false, {this}, EDrawDebugTrace::ForDuration, HitResults, true, FLinearColor::Red, FLinearColor::Green, 2.0f);
		//球体检测
	
		UKismetSystemLibrary::SphereTraceMulti(this, AttackDetectionPointsLocation[i], GetAttackDetectionSocketLocationByTag_Implementation(MontageEventTag)[i], AttackDetectionRadius, TraceTypeQuery1, false, {this}, EDrawDebugTrace::ForDuration, HitResults, true, FLinearColor::Red, FLinearColor::Green, 1.0f);

		
		for(auto HitResult: HitResults)
		{
			if(HitResult.bBlockingHit)
			{
				AActor* HitActor = HitResult.GetActor();
				//如果是敌人而且没死，就加入HitActors
				if(HitActor->Implements<UCombatInterface>() && !Execute_IsDead(HitActor) && !UMageAbilitySystemLibrary::IsFriendly(this,HitActor) )
				{
					//防止多次检测
					if(!HitActors.Contains(HitActor))
					{
						HitActors.AddUnique(HitActor);
					}
				}
			}
		}
	}

	//发送GameplayEvent
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, MontageEventTag,FGameplayEventData());
	
	//更新
	GetAttackDetectionPointsLocation();
}


TArray<FVector> AMageCharacterBase::GetAttackDetectionSocketLocationByTag_Implementation(const FGameplayTag& SocketTag) const
{
	const FMageGameplayTags GameplayTags = FMageGameplayTags::Instance();
	TArray<FVector> AttackDetectionSocketLocations;
	for(auto &Pair:MontageEventTag_To_AttackDetectionSockets)
	{
		FGameplayTag EventTag = Pair.Key;
		const FAttackDetectionSocket AttackDetectionSocket = Pair.Value;

		if(SocketTag.MatchesTagExact(EventTag))
		{
			//如果武器指定了Mesh，就用武器的Socket，否则用Mesh的Socket(比如手部)
			if(IsValid(Weapon->GetSkeletalMeshAsset()))
			{
				for(const auto SocketName : AttackDetectionSocket.AttackDetectionSockets)
				{
					AttackDetectionSocketLocations.Add(Weapon->GetSocketLocation(SocketName));
				}
			}
			else
			{
				for(const auto SocketName : AttackDetectionSocket.AttackDetectionSockets)
				{
					AttackDetectionSocketLocations.Add(GetMesh()->GetSocketLocation(SocketName));
				}
			}
			//TODO:暂不兼容ReratgetMesh
		}
	}
	return AttackDetectionSocketLocations;
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
		if(TaggedMontage.MontageEventTag == MontageTag)
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
