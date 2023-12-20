#include "GAS/MageAbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/MageCharacterBase.h"
#include "Game/MageGameMode.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAbilityTypes.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MagePlayerController.h"
#include "Player/MagePlayerState.h"
#include "UI/HUD/MageHUD.h"
#include "UI/WidgetController/MageWidgetController.h"

FWidgetControllerParams UMageAbilitySystemLibrary::MakeWidgetControllerParams(APlayerController* PC)
{
	if(PC)
	{
		AMagePlayerState* PS = PC->GetPlayerState<AMagePlayerState>();
		UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
		UAttributeSet* AttributeSet = PS->GetAttributeSet();
		return FWidgetControllerParams(PC, PS, ASC, AttributeSet);
	}
	return FWidgetControllerParams();
}

UOverlayWidgetController* UMageAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AMageHUD* MageHUD = Cast<AMageHUD>(PC->GetHUD()))
		{
			return MageHUD->GetOverlayWidgetController(MakeWidgetControllerParams(PC));
		}
	}
	return nullptr;
}

UAttributeMenuWidgetController* UMageAbilitySystemLibrary::GetAttributeMenuWidgetController(
	const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AMageHUD* MageHUD = Cast<AMageHUD>(PC->GetHUD()))
		{
			return MageHUD->GetAttributeMenuWidgetController(MakeWidgetControllerParams(PC));
		}
	}
	return nullptr;
}

USkillTreeWidgetController* UMageAbilitySystemLibrary::GetSkillTreeWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AMageHUD* MageHUD = Cast<AMageHUD>(PC->GetHUD()))
		{
			return MageHUD->GetSkillTreeWidgetController(MakeWidgetControllerParams(PC));
		}
	}
	return nullptr;
}

void UMageAbilitySystemLibrary::ApplyEffectToSelf(UAbilitySystemComponent* ASC,
                                                  TSubclassOf<UGameplayEffect> GameplayEffectClass, const float Level)
{
	checkf(GameplayEffectClass, TEXT("GameplayEffectClass为空，请在 %s 中设置"), *ASC->GetOwner()->GetName());
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(ASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
	const FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(GameplayEffectClass, Level, EffectContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data); //返回FActiveGameplayEffectHandle
}


FGameplayEffectContextHandle UMageAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams)
{
	//创建FGameplayEffectContextHandle
	FGameplayEffectContextHandle EffectContextHandle = DamageEffectParams.SourceASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(DamageEffectParams.SourceASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
	// GameplayEffectContextHandle 可以设置许多关联数据 

	//设置EffectContextHandle中的DeathImpulse
	SetDeathImpulse(EffectContextHandle, DamageEffectParams.DeathImpulse);

	//设置EffectContextHandle中的KnockbackForce
	SetKnockbackForce(EffectContextHandle, DamageEffectParams.KnockbackForce);

	/** 创建 GameplayEffectSpecHandle, 注意这里给GameplayEffectSpec设置了技能等级，后续可通过GetLevel获取 */
	const FGameplayEffectSpecHandle EffectSpecHandle = DamageEffectParams.SourceASC->MakeOutgoingSpec(
		DamageEffectParams.DamageGameplayEffectClass, DamageEffectParams.AbilityLevel, EffectContextHandle);

	/**
	 * 使用Set By Caller Modifier 从曲线表格中获取技能类型伤害和Debuff伤害，在ExecCalc_Damage中Get对应值
	 */
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, DamageEffectParams.DamageTypeTag,
	                                                              DamageEffectParams.BaseDamage);
	
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle,
	                                                              MageGameplayTags.Debuff_Params_Chance,
	                                                              DamageEffectParams.DebuffChance);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle,
	                                                              MageGameplayTags.Debuff_Params_Damage,
	                                                              DamageEffectParams.DebuffDamage);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle,
	                                                              MageGameplayTags.Debuff_Params_Frequency,
	                                                              DamageEffectParams.DebuffFrequency);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle,
	                                                              MageGameplayTags.Debuff_Params_Duration,
	                                                              DamageEffectParams.DebuffDuration);

	/** 应用GE Spec（注意，要先分配SetByCaller再Apply） */
	DamageEffectParams.TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data);

	return EffectContextHandle;
}

bool UMageAbilitySystemLibrary::GetIsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetIsCriticalHit();
	}
	return false;
}

void UMageAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& EffectContextHandle,
                                                 const bool bIsCriticalHit)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get())) //注意这里不能用Cast, Cast不能用于结构体
	{
		MageEffectContext->SetIsCriticalHit(bIsCriticalHit);
	}
}

bool UMageAbilitySystemLibrary::GetIsDebuff(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetIsDebuff();
	}
	return false;
}

void UMageAbilitySystemLibrary::SetIsDebuff(FGameplayEffectContextHandle& EffectContextHandle, const bool bIsDebuff)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetIsDebuff(bIsDebuff);
	}
}

float UMageAbilitySystemLibrary::GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetDebuffDamage();
	}
	return -1;
}

void UMageAbilitySystemLibrary::SetDebuffDamage(FGameplayEffectContextHandle& EffectContextHandle,
                                                const float DebuffDamage)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetDebuffDamage(DebuffDamage);
	}
}

float UMageAbilitySystemLibrary::GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetDebuffFrequency();
	}
	return -1;
}

void UMageAbilitySystemLibrary::SetDebuffFrequency(FGameplayEffectContextHandle& EffectContextHandle,
                                                   const float DebuffFrequency)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetDebuffFrequency(DebuffFrequency);
	}
}

float UMageAbilitySystemLibrary::GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetDebuffDuration();
	}
	return -1;
}

void UMageAbilitySystemLibrary::SetDebuffDuration(FGameplayEffectContextHandle& EffectContextHandle,
                                                  const float DebuffDuration)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetDebuffDuration(DebuffDuration);
	}
}

FGameplayTag UMageAbilitySystemLibrary::GetDamageTypeTag(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		if (MageEffectContext->GetDamageTypeTag().IsValid())
		{
			return *MageEffectContext->GetDamageTypeTag();
		}
	}
	return FGameplayTag::EmptyTag;
}

void UMageAbilitySystemLibrary::SetDamageTypeTag(FGameplayEffectContextHandle& EffectContextHandle,
                                                 const FGameplayTag& DamageTypeTag)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetDamageTypeTag(MakeShared<FGameplayTag>(DamageTypeTag));
	}
}

FVector UMageAbilitySystemLibrary::GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetDeathImpulse();
	}
	return FVector::ZeroVector;
}

void UMageAbilitySystemLibrary::SetDeathImpulse(FGameplayEffectContextHandle& EffectContextHandle,
                                                const FVector DeathImpulse)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetDeathImpulse(DeathImpulse);
	}
}

FVector UMageAbilitySystemLibrary::GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FMageGameplayEffectContext* MageEffectContext = static_cast<const FMageGameplayEffectContext*>(
		EffectContextHandle.Get()))
	{
		return MageEffectContext->GetKnockbackForce();
	}
	return FVector::ZeroVector;
}

void UMageAbilitySystemLibrary::SetKnockbackForce(FGameplayEffectContextHandle& EffectContextHandle,
                                                  const FVector KnockbackForce)
{
	if (FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(EffectContextHandle.
		Get()))
	{
		MageEffectContext->SetKnockbackForce(KnockbackForce);
	}
}

AActor* UMageAbilitySystemLibrary::GetAvatarActorFromASC(UAbilitySystemComponent* ASC)
{
	if(ASC)
	{
		return ASC->GetAvatarActor();
	}
	return nullptr;
}

void UMageAbilitySystemLibrary::GiveCharacterAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC,
                                                       ECharacterClass CharacterClass)
{
	UCharacterClassDataAsset* CharacterClassDataAsset = GetCharacterClassDataAsset(WorldContextObject);
	if (CharacterClassDataAsset == nullptr) return;

	/** 授予全部的公共Abilities */
	for (const auto CommonAbility : CharacterClassDataAsset->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(CommonAbility);
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->StartupAbilityLevel; //设置技能等级

			/** 授予Ability */
			ASC->GiveAbility(AbilitySpec); //授予后不激活
			//GiveAbilityAndActivateOnce(AbilitySpec); //授予并立即激活一次
		}

#pragma region  另一种方式
		// if (UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(CommonAbility.GetDefaultObject()))
		// {
		// 	FGameplayAbilitySpec AbilitySpec(MageGameplayAbility, MageGameplayAbility->AbilityLevel); //设置技能等级
		// 	
		// 	/** 授予Ability */
		// 	ASC->GiveAbility(AbilitySpec); //授予后不激活
		// }
#pragma endregion
	}

	/** 授予全部的特有Abilities */
	const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetCharacterClassDefaultInfo(
		CharacterClass);

	for (const auto BaseAbility : CharacterClassDefaultInfo.BaseAbilities)
	{
		FGameplayAbilitySpec AbilitySpec(BaseAbility);
		if (const UMageGameplayAbility* MageGameplayAbility = Cast<UMageGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.Level = MageGameplayAbility->StartupAbilityLevel; //设置技能等级

			/** 授予Ability */
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

float UMageAbilitySystemLibrary::GetScalableFloatValueAtLevel(const FScalableFloat& ScalableFloat, const int32 Level)
{
	return ScalableFloat.GetValueAtLevel(Level);
}

void UMageAbilitySystemLibrary::GetAbilityLayerByTimeHeld(EExecNodePin& Result, const float TimeHeld)
{
	if(TimeHeld <= 1)
	{
		Result = EExecNodePin::Layer1;
	}
	else if(TimeHeld > 1 && TimeHeld <= 2)
	{
		Result = EExecNodePin::Layer2;
	}
	else if(TimeHeld > 2 && TimeHeld <= 4)
	{
		Result = EExecNodePin::Layer3;
	}
	else if(TimeHeld > 4)
	{
		Result = EExecNodePin::Layer4;
	}
}

int32 UMageAbilitySystemLibrary::GetAbilityLevelFromTag(UAbilitySystemComponent* ASC, const FGameplayTag AbilityTag)
{
	TArray<FGameplayAbilitySpecHandle> OutAbilityHandles;
	FGameplayTagContainer Tags;
	Tags.AddTag(AbilityTag);

	ASC->FindAllAbilitiesWithTags(OutAbilityHandles, Tags);
	int32 AbilityLevel = 0;
	for (const auto AbilityHandle : OutAbilityHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(AbilityHandle))
		{
			if (const UMageGameplayAbility* MageGA = Cast<UMageGameplayAbility>(AbilitySpec->Ability))
			{
				AbilityLevel = MageGA->GetAbilityLevel();
			}
		}
	}
	return AbilityLevel;
}

bool UMageAbilitySystemLibrary::IsFriendly(AActor* FirstActor, AActor* SecondActor)
{
	const IGameplayTagAssetInterface* FirstTagAssetInterface = Cast<IGameplayTagAssetInterface>(FirstActor);
	const IGameplayTagAssetInterface* SecondTagAssetInterface = Cast<IGameplayTagAssetInterface>(SecondActor);
	if (FirstTagAssetInterface && SecondTagAssetInterface)
	{
		const FGameplayTag CharacterTag = FMageGameplayTags::Instance().Character_Player;

		const bool bFirstIsPlayer = FirstTagAssetInterface->HasMatchingGameplayTag(CharacterTag);
		const bool bSecondIsPlayer = SecondTagAssetInterface->HasMatchingGameplayTag(CharacterTag);

		if (bFirstIsPlayer == bSecondIsPlayer)
		{
			return true;
		}
	}
	return false;
}


void UMageAbilitySystemLibrary::InitDefaultAttributesByCharacterClass(const UObject* WorldContextObject,
                                                      const ECharacterClass CharacterClass, const int32 Level,
                                                      UAbilitySystemComponent* ASC)
{
	/** 使用GE初始化Attribute */
	if (UCharacterClassDataAsset* CharacterClassDataAsset = GetCharacterClassDataAsset(WorldContextObject))
	{
		const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetCharacterClassDefaultInfo(
			CharacterClass);

		ApplyEffectToSelf(ASC, CharacterClassDefaultInfo.PrimaryAttribute.Get(), Level);
		ApplyEffectToSelf(ASC, CharacterClassDataAsset->VitalAttribute.Get(), Level);
		// VitalAttribute基于SecondaryAttribute生成初始值，所以先让SecondaryAttribute初始化
		ApplyEffectToSelf(ASC, CharacterClassDataAsset->ResistanceAttribute.Get(), Level);
	}
}

UCharacterClassDataAsset* UMageAbilitySystemLibrary::GetCharacterClassDataAsset(const UObject* WorldContextObject)
{
	/** GameMode仅服务器有效, 若其他函数调用它需要检查 if(HasAuthority()) */
	if (const AMageGameMode* MageGameMode = Cast<AMageGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		checkf(MageGameMode->CharacterClassDataAsset, TEXT("CharacterClassDataAsset为空, 请在 MageGameMode 中设置"));

		return MageGameMode->CharacterClassDataAsset;
	}
	return nullptr;
}

UAbilityDataAsset* UMageAbilitySystemLibrary::GetAbilityDataAsset(const UObject* WorldContextObject)
{
	/** GameMode仅服务器有效, 若其他函数调用它需要检查 if(HasAuthority()) */
	if (const AMageGameMode* MageGameMode = Cast<AMageGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
	{
		checkf(MageGameMode->AbilityDataAsset, TEXT("AbilityDataAsset为空, 请在 MageGameMode 中设置"));

		return MageGameMode->AbilityDataAsset;
	}
	return nullptr;
}

int32 UMageAbilitySystemLibrary::GetExpRewardForClassAndLevel(const UObject* WorldContextObject,
                                                              ECharacterClass CharacterClass,
                                                              const int32 CharacterLevel)
{
	if (UCharacterClassDataAsset* CharacterClassDataAsset = GetCharacterClassDataAsset(WorldContextObject))
	{
		const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetCharacterClassDefaultInfo(
			CharacterClass);

		float ExpReward = CharacterClassDefaultInfo.ExpReward.GetValueAtLevel(CharacterLevel);

		return static_cast<int32>(ExpReward);
	}

	return 0;
}

void UMageAbilitySystemLibrary::GetLivingActorInCollisionShape(const UObject* WorldContextObject,
                                                               TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& IgnoreActors, const FVector& Origin,
                                                               const EColliderShape ColliderShape, const bool Debug, const float SphereRadius, const FVector BoxHalfExtent, 
                                                               const float CapsuleRadius, const float CapsuleHalfHeight)
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(IgnoreActors);

	// 查询场景，看看Hit了什么
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<FOverlapResult> Overlaps;

		FCollisionShape CollisionShape;
		
		if(ColliderShape == EColliderShape::Sphere)
		{
			CollisionShape = FCollisionShape::MakeSphere(SphereRadius);
			if(Debug)
			{
				DrawDebugSphere(World,Origin,SphereRadius,12,FColor::Red,false,1.0f,0,2.0f);
			}
		}
		else if(ColliderShape == EColliderShape::Box)
		{
			CollisionShape = FCollisionShape::MakeBox(BoxHalfExtent);
			if(Debug)
			{
				DrawDebugBox(World,Origin,BoxHalfExtent,FColor::Red,false,1.0f,0,2.0f);
			}
		}
		else if(ColliderShape == EColliderShape::Capsule)
		{
			CollisionShape = FCollisionShape::MakeCapsule(CapsuleRadius,CapsuleHalfHeight);
			if(Debug)
			{
				DrawDebugCapsule(World,Origin,CapsuleHalfHeight,CapsuleRadius,FQuat::Identity,FColor::Red,false,1.0f,0,2.0f);
			}
		}
		
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), CollisionShape, QueryParams);
		
		for (FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.GetActor()->Implements<UCombatInterface>())
			{
				const bool IsDead = ICombatInterface::Execute_IsDead(Overlap.GetActor());
				if (!IsDead)
				{
					OutOverlappingActors.AddUnique(Overlap.GetActor());
				}	
			}
		}
	}
}

void UMageAbilitySystemLibrary::GetLivingEnemyInCollisionShape(const UObject* WorldContextObject,AActor* OwnerActor,
	TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& IgnoreActors, const FVector& Origin,
	const EColliderShape ColliderShape, const bool Debug, const float SphereRadius, const FVector BoxHalfExtent,
	const float CapsuleRadius, const float CapsuleHalfHeight)
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(IgnoreActors);

	// 查询场景，看看Hit了什么
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<FOverlapResult> Overlaps;

		FCollisionShape CollisionShape;
		
		if(ColliderShape == EColliderShape::Sphere)
		{
			CollisionShape = FCollisionShape::MakeSphere(SphereRadius);
			if(Debug)
			{
				DrawDebugSphere(World,Origin,SphereRadius,12,FColor::Red,false,1.0f,0,2.0f);
			}
		}
		else if(ColliderShape == EColliderShape::Box)
		{
			CollisionShape = FCollisionShape::MakeBox(BoxHalfExtent);
			if(Debug)
			{
				DrawDebugBox(World,Origin,BoxHalfExtent,FColor::Red,false,1.0f,0,2.0f);
			}
		}
		else if(ColliderShape == EColliderShape::Capsule)
		{
			CollisionShape = FCollisionShape::MakeCapsule(CapsuleRadius,CapsuleHalfHeight);
			if(Debug)
			{
				DrawDebugCapsule(World,Origin,CapsuleHalfHeight,CapsuleRadius,FQuat::Identity,FColor::Red,false,1.0f,0,2.0f);
			}
		}
		
		World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), CollisionShape, QueryParams);
		
		for (FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.GetActor()->Implements<UCombatInterface>() && !IsFriendly(Overlap.GetActor(),OwnerActor))
			{
				const bool IsDead = ICombatInterface::Execute_IsDead(Overlap.GetActor());
				if (!IsDead)
				{
					OutOverlappingActors.AddUnique(Overlap.GetActor());
				}	
			}
		}
	}
}

AActor* UMageAbilitySystemLibrary::GetClosestActor(const TArray<AActor*>& CheckedActors,
                                                   const FVector& Origin)
{
	if(CheckedActors.IsEmpty())
	{
		return nullptr;
	}
	
	// 建立Actor到距Origin距离的Map
	TMap<AActor*,float> Actor_To_DistanceToOrigin;
	for (auto Target : CheckedActors)
	{
		Actor_To_DistanceToOrigin.Add(Target, FVector::Distance(Target->GetActorLocation(), Origin));
	}

	// 按距离升序排列
	Actor_To_DistanceToOrigin.ValueSort([](const float& A, const float& B) { return A < B; });

	// 返回map第一个Key，即距离Origin最近的Actor
	return Actor_To_DistanceToOrigin.begin()->Key;
}


void UMageAbilitySystemLibrary::GetClosestActors(const TArray<AActor*>& CheckedActors,
                                                 TArray<AActor*>& OutClosestActors, const FVector& Origin, const int32 MaxTargetNum)
{
	if(CheckedActors.Num()<=MaxTargetNum)
	{
		OutClosestActors = CheckedActors;
		return;
	}
	
	// 建立Actor到距Origin距离的Map
	TMap<AActor*,float> Actor_To_DistanceToOrigin;
	for (auto Target : CheckedActors)
	{
		Actor_To_DistanceToOrigin.Add(Target, FVector::Distance(Target->GetActorLocation(), Origin));
	}

	// 按距离升序排列
	Actor_To_DistanceToOrigin.ValueSort([](const float& A, const float& B) { return A < B; });

	// 遍历Map，取出前几个Actor(距离Origin最近的Actor)
	for(auto Pair : Actor_To_DistanceToOrigin)
	{
		if(OutClosestActors.Num() >= MaxTargetNum)
		{
			break;
		}
		OutClosestActors.Add(Pair.Key);
	}
}

AActor* UMageAbilitySystemLibrary::GetTargetingActor(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if(const AMagePlayerController* MagePC = Cast<AMagePlayerController>(PC))
		{
			return MagePC->GetTargetingActor();
		}
	}
	return nullptr;
}

FVector UMageAbilitySystemLibrary::GetMagicCircleLocation(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if(const AMagePlayerController* MagePC = Cast<AMagePlayerController>(PC))
		{
			return MagePC->GetMagicCircleLocation();
		}
	}
	return FVector::ZeroVector;
}


TArray<FRotator> UMageAbilitySystemLibrary::EvenlySpacedRotators(const FVector& Forward, const FVector& Axis,
                                                                 const float SpreadAngle, const int32 SpreadNum)
{
	TArray<FRotator> Rotators;
	if (SpreadNum > 1)
	{
		// 绕轴旋转到角度最左边(Z轴（UpVector）旋转方向遵循左手定则，顺时针为正)
		const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpreadAngle / 2.f, Axis);
		const float DeltaSpread = SpreadAngle / (SpreadNum - 1); //每个投掷物之间的角度差(例如有三个投掷物时，间隔为45度)
		for (int32 i = 0; i < SpreadNum; i++)
		{
			const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i, Axis);
			Rotators.Add(Direction.ToOrientationRotator());
		}
	}
	else //只有一个投掷物时，只保存Forward
	{
		Rotators.Add(Forward.ToOrientationRotator());
	}

	return Rotators;
}

TArray<FVector> UMageAbilitySystemLibrary::EvenlySpacedVectors(const FVector& Forward, const FVector& Axis,
                                                               const float SpreadAngle, const int32 SpreadNum)
{
	TArray<FVector> Vectors;
	if (SpreadNum > 1)
	{
		// 绕轴旋转到角度最左边(Z轴（UpVector）旋转方向遵循左手定则，顺时针为正)
		const FVector LeftOfSpread = Forward.RotateAngleAxis(-SpreadAngle / 2.f, Axis);
		const float DeltaSpread = SpreadAngle / (SpreadNum - 1); //每个投掷物之间的角度差(例如有三个投掷物时，间隔为45度)
		for (int32 i = 0; i < SpreadNum; i++)
		{
			const FVector Direction = LeftOfSpread.RotateAngleAxis(DeltaSpread * i, Axis);
			Vectors.Add(Direction);
		}
	}
	else //只有一个投掷物时，只保存Forward
	{
		Vectors.Add(Forward);
	}

	return Vectors;
}
