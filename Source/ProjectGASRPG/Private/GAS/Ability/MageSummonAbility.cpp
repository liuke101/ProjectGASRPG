#include "GAS/Ability/MageSummonAbility.h"

TArray<FVector> UMageSummonAbility::GetSpawnLocations() const
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();
	const float DeltaSpread = SpawnSpread / SummonCount; //每个召唤物之间的角度差
	
	// 绕轴旋转(Z轴（UpVector）旋转方向遵循左手定则，顺时针为正)
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread/2.f, FVector::UpVector);
	TArray<FVector> SpawnLocations;
	
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
	
	return SpawnLocations;
}
