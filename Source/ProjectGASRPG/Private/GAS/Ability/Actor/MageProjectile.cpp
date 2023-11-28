#include "GAS/Ability/Actor/MageProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AudioDevice.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectGASRPG/ProjectGASRPG.h"


AMageProjectile::AMageProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SetRootComponent(Sphere);
	/** 设置碰撞 */
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_Projectile);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);


	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 550.f;
	ProjectileMovement->MaxSpeed = 550.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AMageProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);

	/** 绑定碰撞委托 */
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AMageProjectile::OnSphereBeginOverlap);

	/** 播放飞行音效 */
	FlyAudioComponent = UGameplayStatics::SpawnSoundAttached(FlySound, RootComponent);
}

void AMageProjectile::Destroyed()
{
	/**
	 * 客户端上的 Projectile 的销毁有两种情况要处理, 用bHit做客户端标记：
	 * 1. 先overlap再Destroyed
	 * 2. 先Destroyed再overlap
	 * /
	
	/** 如果客户端上的 Projectile 在重叠前被Destroyed */
	if (!bHit && !HasAuthority())
	{
		OnHit();
	}
	
	DestroyFlyAudioComponent();
	Super::Destroyed();
}

void AMageProjectile::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                           const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		AActor* SourceAvatarActor = DamageEffectParams.SourceASC->GetAvatarActor();

		//Overlap的是自己或者友方, 不做处理
		if (SourceAvatarActor == OtherActor || UMageAbilitySystemLibrary::IsFriendly(SourceAvatarActor, OtherActor)) return;
	}

	if (!bHit)
	{
		OnHit();
	}

	if (HasAuthority())
	{
		if (UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			// 死亡冲量
			const FVector DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
			DamageEffectParams.DeathImpulse = DeathImpulse; // 设置DeathImpulse
			DamageEffectParams.TargetASC = OtherASC; // 设置TargetASC

			// 击退
			const bool bKnockback = FMath::RandRange(0.0f,1.0f) <= DamageEffectParams.KnockbackChance;
			if(bKnockback)
			{
				FRotator Rotation = GetActorRotation();
				Rotation.Pitch = 45.0f;
				const FVector KnockbackDirection = Rotation.Vector();
				
				const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
				DamageEffectParams.KnockbackForce = KnockbackForce; // 设置KnockbackForce
			}

			// 对 OtherActor 造成伤害
			UMageAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams); 
		}
		Destroy(); // 服务端销毁该Actor, 也会通知客户端销毁Actor
	}
	else
	{
		bHit = true;
	}
}

void AMageProjectile::OnHit()
{
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());

	DestroyFlyAudioComponent();

	bHit = true;
}

void AMageProjectile::DestroyFlyAudioComponent() const
{
	if (FlyAudioComponent)
	{
		FlyAudioComponent->Stop();
		FlyAudioComponent->DestroyComponent();
	}
}
