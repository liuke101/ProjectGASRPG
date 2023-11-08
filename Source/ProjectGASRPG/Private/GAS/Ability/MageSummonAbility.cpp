#include "GAS/Ability/MageSummonAbility.h"

TArray<FVector> UMageSummonAbility::GetSpawnLocations() const
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();
	const float DeltaSpread = SpawnSpread / SummonCount;
	
	// 绕轴旋转(Z轴（UpVector）旋转方向遵循左手定则，顺时针为正)
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpawnSpread/2.f, FVector::UpVector);
	TArray<FVector> SpawnLocations;
	
	for(int32 i =0;i<SummonCount;i++)
	{
		const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i, FVector::UpVector);
		const FVector SpawnLocation = Location + Direction * FMath::RandRange(MinSpawnDistance, MaxSpawnDistance);
		SpawnLocations.Add(SpawnLocation);

		DrawDebugSphere(GetWorld(),Location+Direction*MinSpawnDistance,10.f,8,FColor::Red,false,5.f);
		DrawDebugSphere(GetWorld(),Location+Direction*MaxSpawnDistance,10.f,8,FColor::Green,false,5.f);

		DrawDebugSphere(GetWorld(),SpawnLocation,10.f,8,FColor::Yellow,false,5.f);
	}
	
	return SpawnLocations;
}
