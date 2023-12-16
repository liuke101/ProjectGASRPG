// 


#include "GAS/Ability/TargetActor/MageTargetActor_LocationRelease.h"

#include "Abilities/GameplayAbility.h"


AMageTargetActor_LocationRelease::AMageTargetActor_LocationRelease()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMageTargetActor_LocationRelease::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	PrimaryPC = Cast<APlayerController>(Ability->GetOwningActorFromActorInfo()->GetInstigatorController());
	SourceActor = Ability->GetOwningActorFromActorInfo();

	ConePoint = GetWorld()->SpawnActor<AActor>(ConePointClass, FVector::ZeroVector, FRotator::ZeroRotator);
}

void AMageTargetActor_LocationRelease::ConfirmTargetingAndContinue()
{
	// ViewLocation是生成碰撞检测的位置
	FVector ViewLocation;
	GetPlayerLookingPoint(ViewLocation);

	TArray<FOverlapResult> OverlapResults;
	TArray<TWeakObjectPtr<AActor>> OverlapActors;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;
	if(SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceActor->GetUniqueID());
	}

	//以观察点为中心 球形检测
	const bool TryOverlap = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		ViewLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	if(TryOverlap)
	{
		for(FOverlapResult& OverlapResult: OverlapResults)
		{
			if(APawn* PawnOverlapped = Cast<APawn>(OverlapResult.GetActor()))
			{
				OverlapActors.AddUnique(PawnOverlapped);
			}
		}
	}

	FVector MeteorSpawnLocation = ConePoint->GetActorLocation();
	MeteorSpawnLocation += ConePoint->GetActorUpVector() * 100.0f;
	//生成技能产生的actor
	Meteor = GetWorld()->SpawnActor<AActor>(MeteorClass, MeteorSpawnLocation, ConePoint->GetActorRotation());
	ConePoint->Destroy(); //销毁指示点

	OverlapActors.Add(Meteor);
	const FGameplayAbilityTargetDataHandle TargetData =  StartLocation.MakeTargetDataHandleFromActors(OverlapActors);
	TargetDataReadyDelegate.Broadcast(TargetData);
}

bool AMageTargetActor_LocationRelease::GetPlayerLookingPoint(FVector& LookingPoint)
{
	// Get Player View Vector and View Rotation
	FVector ViewVector;
	FRotator ViewRotation;
	if(PrimaryPC)
	{
		//获取视线方向和位置,这里可以通过获取 Character 的 Camera 的位置和方向来替代这个函数。
		PrimaryPC->GetPlayerViewPoint(ViewVector, ViewRotation);
	}
	
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	if(SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceActor->GetUniqueID());
	}
	
	//从视线方向，射线检测
	FHitResult HitResult;
	bool TryTrace = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		ViewVector,
		ViewVector + ViewRotation.Vector()*10000.0f,
		ECollisionChannel::ECC_Visibility,
		QueryParams);

	if(TryTrace)
	{
		LookingPoint = HitResult.ImpactPoint; //获取碰撞点
	}
	else
	{
		LookingPoint = FVector::ZeroVector;
	}

	return TryTrace;
}

void AMageTargetActor_LocationRelease::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//不断更新技能释放指示点
	FVector LookingPoint;
	if(GetPlayerLookingPoint(LookingPoint))
	{
		ConePoint->SetActorLocation(LookingPoint);
	}
}

