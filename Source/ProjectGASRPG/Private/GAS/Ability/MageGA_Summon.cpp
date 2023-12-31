﻿#include "GAS/Ability/MageGA_Summon.h"

TArray<FVector> UMageGA_Summon::GetSpawnLocations() const
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();
	
	// 绕轴旋转(Z轴（UpVector）旋转方向遵循左手定则，顺时针为正)
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread/2.f, FVector::UpVector);
	TArray<FVector> SpawnLocations;

	if(SummonCount>0)
	{
		float DeltaSpread = SpawnSpread; //每个召唤物之间的角度差
		if(SummonCount>1)
		{
			DeltaSpread = SpawnSpread / (SummonCount - 1); //例如有三个召唤物时，间隔为45度
		}
		
		for(int32 i =0;i<SummonCount;i++)
		{
			const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i, FVector::UpVector);
			FVector SpawnLocation = Location + Direction * FMath::RandRange(MinSpawnDistance, MaxSpawnDistance);
			
			/** 通过射线检测, 保证召唤物生成在地面上 */
			FHitResult HitResult;
			GetWorld()->LineTraceSingleByChannel(HitResult,SpawnLocation + FVector(0,0,400),SpawnLocation - FVector(0,0,400),ECC_Visibility);
			if(HitResult.bBlockingHit)
			{
				SpawnLocation = HitResult.ImpactPoint;
			}
			
			SpawnLocations.Add(SpawnLocation);
		}	
}
	
	return SpawnLocations;
}

TSubclassOf<APawn> UMageGA_Summon::GetRandomSummonClass() const
{
	return SummonClasses[FMath::RandRange(0, SummonClasses.Num() - 1)];
}
