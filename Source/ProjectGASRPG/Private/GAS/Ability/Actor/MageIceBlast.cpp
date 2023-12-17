// 


#include "GAS/Ability/Actor/MageIceBlast.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectGASRPG/ProjectGASRPG.h"
#include "Tasks/Task.h"


AMageIceBlast::AMageIceBlast()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	/** 设置碰撞 */
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);  //改为queryOnly触发
	CapsuleComponent->SetCollisionObjectType(ECC_Target);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	
}

void AMageIceBlast::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpan);
	
	//生成Niagara
	if(AbilityNiagara)
	{
		//UNiagaraSystem* NiagaraSystem = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AbilityNiagara, GetActorTransform());
	}
	
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AMageIceBlast::OnCapsuleBeginOverlap);
}

void AMageIceBlast::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMageIceBlast::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (DamageEffectParams.SourceASC == nullptr) return;
	
	AActor* SourceAvatarActor = DamageEffectParams.SourceASC->GetAvatarActor();

	//Overlap的是自己或者友方, 不做处理
	if (SourceAvatarActor == OtherActor || UMageAbilitySystemLibrary::IsFriendly(SourceAvatarActor, OtherActor)) return;

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
		
		Destroy();
	}
}