#include "GAS/Ability/MageGA_Damage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "Interface/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectGASRPG/ProjectGASRPG.h"

FDamageEffectParams UMageGA_Damage::MakeDamageEffectParamsFromClassDefault(AActor* TargetActor) const
{
	FDamageEffectParams Params;
	Params.WorldContextObject = GetAvatarActorFromActorInfo();
	Params.DamageGameplayEffectClass = DamageEffectClass;
	Params.SourceASC = GetAbilitySystemComponentFromActorInfo();
	Params.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	Params.AbilityLevel = GetAbilityLevel();
	Params.BaseDamage = TypeDamage.GetValueAtLevel(Params.AbilityLevel);
	Params.DamageTypeTag = DamageTypeTag;
	Params.DebuffChance = DebuffChance;
	Params.DebuffDamage = DebuffDamage;
	Params.DebuffFrequency = DebuffFrequency;
	Params.DebuffDuration = DebuffDuration;
	Params.DeathImpulseMagnitude = DeathImpulseMagnitude;
	Params.KnockbackForceMagnitude = KnockbackForceMagnitude;
	Params.KnockbackChance = KnockbackChance;

	if(IsValid(TargetActor))
	{
		FRotator Rotation =  (TargetActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation()).ToOrientationRotator();
		Rotation.Pitch = 45.0f;
		const FVector ToTarget = Rotation.Vector();

		Params.DeathImpulse = ToTarget * DeathImpulseMagnitude;
		Params.KnockbackForce = ToTarget * KnockbackForceMagnitude;
	}

	return Params;
}

void UMageGA_Damage::GetTargetingActorInfo()
{
	TargetingActor = UMageAbilitySystemLibrary::GetTargetingActor(GetAvatarActorFromActorInfo());
	if(TargetingActor)
	{
		TargetingActorLocation = TargetingActor->GetActorLocation();
	}
	else
	{
		//如果没有目标，允许向其他任意地方释放技能
		FHitResult CursorHit;
		const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
		if (CursorHit.bBlockingHit)
		{
			TargetingActor = CursorHit.GetActor();
			TargetingActorLocation = CursorHit.ImpactPoint;  //或者CursorHit.Location
		}
	}
}



float UMageGA_Damage::GetTypeDamage_Implementation(
	const int32 AbilityLevel) const
{
	return TypeDamage.GetValueAtLevel(AbilityLevel);
}


