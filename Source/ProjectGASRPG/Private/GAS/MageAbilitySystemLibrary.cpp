#include "GAS/MageAbilitySystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Game/MageGameMode.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAbilityTypes.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Ability/MageGameplayAbility.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/MagePlayerState.h"
#include "UI/HUD/MageHUD.h"
#include "UI/WidgetController/MageWidgetController.h"

FWidgetControllerParams UMageAbilitySystemLibrary::MakeWidgetControllerParams(APlayerController* PC)
{
	AMagePlayerState* PS = PC->GetPlayerState<AMagePlayerState>();
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	UAttributeSet* AttributeSet = PS->GetAttributeSet();
	return FWidgetControllerParams(PC, PS, ASC, AttributeSet);
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
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(ASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
	const FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(
		GameplayEffectClass, Level, EffectContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data); //返回FActiveGameplayEffectHandle
}


FGameplayEffectContextHandle UMageAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams)
{
	FGameplayEffectContextHandle EffectContextHandle = DamageEffectParams.SourceASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(DamageEffectParams.SourceASC->GetAvatarActor()); //添加源对象，计算MMC时会用到
	// GameplayEffectContextHandle 可以设置许多关联数据 
	// EffectContextHandle.SetAbility(this);
	// TArray<TWeakObjectPtr<AActor>> Actors;
	// Actors.Add(XXX);
	// EffectContextHandle.AddActors(Actors);
	// FHitResult HitResult;
	// HitResult.Location = TargetLocation;
	// EffectContextHandle.AddHitResult(HitResult);

	/** 创建 GameplayEffectSpecHandle, 注意这里给GameplayEffectSpec设置了技能等级，后续可通过GetLevel获取 */
	const FGameplayEffectSpecHandle EffectSpecHandle = DamageEffectParams.SourceASC->MakeOutgoingSpec(
		DamageEffectParams.DamageGameplayEffectClass, DamageEffectParams.AbilityLevel, EffectContextHandle);

	/**
	 * 使用Set By Caller Modifier 从曲线表格中获取技能类型伤害和Debuff伤害
	 * - AssignTagSetByCallerMagnitude 设置 DamageTypeTag 对应的 magnitude
	 */
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, DamageEffectParams.DamageTypeTag,
	                                                              DamageEffectParams.BaseDamage);
	const FMageGameplayTags MageGameplayTags = FMageGameplayTags::Get();
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

void UMageAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& EffectContextHandle, const bool bIsCriticalHit)
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

void UMageAbilitySystemLibrary::SetDebuffDamage(FGameplayEffectContextHandle& EffectContextHandle, const float DebuffDamage)
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

void UMageAbilitySystemLibrary::SetDebuffFrequency(FGameplayEffectContextHandle& EffectContextHandle, const float DebuffFrequency)
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
		if(MageEffectContext->GetDamageTypeTag().IsValid())
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
	const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetClassDefaultInfo(
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
		const FGameplayTag CharacterTag = FMageGameplayTags::Get().Character_Player;

		const bool bFirstIsPlayer = FirstTagAssetInterface->HasMatchingGameplayTag(CharacterTag);
		const bool bSecondIsPlayer = SecondTagAssetInterface->HasMatchingGameplayTag(CharacterTag);

		if (bFirstIsPlayer == bSecondIsPlayer)
		{
			return true;
		}
	}
	return false;
}


void UMageAbilitySystemLibrary::InitDefaultAttributes(const UObject* WorldContextObject,
                                                      const ECharacterClass CharacterClass, const int32 Level,
                                                      UAbilitySystemComponent* ASC)
{
	/** 使用GE初始化Attribute */
	if (UCharacterClassDataAsset* CharacterClassDataAsset = GetCharacterClassDataAsset(WorldContextObject))
	{
		const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetClassDefaultInfo(
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
		const FCharacterClassDefaultInfo CharacterClassDefaultInfo = CharacterClassDataAsset->GetClassDefaultInfo(
			CharacterClass);

		float ExpReward = CharacterClassDefaultInfo.ExpReward.GetValueAtLevel(CharacterLevel);

		return static_cast<int32>(ExpReward);
	}

	return 0;
}

void UMageAbilitySystemLibrary::GetLivePlayerWithInRadius(const UObject* WorldContextObject,
                                                          TArray<AActor*>& OutOverlappingActors,
                                                          const TArray<AActor*>& IgnoreActors,
                                                          const FVector& SphereOrigin,
                                                          const float Radius)
{
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(IgnoreActors);

	// 查询场景，看看Hit了什么
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,
	                                                             EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<FOverlapResult> Overlaps;

		World->OverlapMultiByObjectType(Overlaps, SphereOrigin, FQuat::Identity,
		                                FCollisionObjectQueryParams(
			                                FCollisionObjectQueryParams::InitType::AllDynamicObjects),
		                                FCollisionShape::MakeSphere(Radius), SphereParams);

		for (FOverlapResult& Overlap : Overlaps)
		{
			/** 用另一种方法检测接口的实现 */
			if (Overlap.GetActor()->Implements<UCombatInterface>())
			// 检查Hit到的Actor是否实现了CombatInterface, 注意是UCombatInterface
			{
				// 如果实现了CombatInterface且没有死亡，就添加到OutOverlappingActors
				const bool IsDead = ICombatInterface::Execute_IsDead(Overlap.GetActor());
				//注意这里：BlueprintNativeEvent标记的函数，执行时为static，可以全局调用
				if (!IsDead)
				{
					OutOverlappingActors.AddUnique(Overlap.GetActor());
				}
			}
		}
	}
}
