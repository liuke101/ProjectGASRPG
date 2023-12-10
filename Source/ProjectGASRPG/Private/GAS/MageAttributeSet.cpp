#include "GAS/MageAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GAS/MageAbilitySystemComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/CombatInterface.h"
#include "Interface/PlayerInterface.h"
#include "Player/MagePlayerController.h"

UMageAttributeSet::UMageAttributeSet()
{
	/** 初始化Map */
#pragma region AttributeTag_To_GetAttributeFuncPtr
	
	/** Vital Attributes */
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Vital_Health, GetHealthAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Vital_Mana, GetManaAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Vital_Vitality, GetVitalityAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Vital_MoveSpeed, GetMoveSpeedAttribute);
	
	/** Primary Attributes */
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Primary_Strength, GetStrengthAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Primary_Intelligence, GetIntelligenceAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Primary_Stamina, GetStaminaAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Primary_Vigor, GetVigorAttribute);

	/** Secondary Attributes */
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MaxHealth, GetMaxHealthAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MaxMana, GetMaxManaAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MaxVitality, GetMaxVitalityAttribute);
	// 应该先添加最大攻，再添加最小攻，这样遍历时保证先修改最大攻（匹配WBP_AttributeRow的更新逻辑）
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MaxPhysicalAttack, GetMaxPhysicalAttackAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MinPhysicalAttack, GetMinPhysicalAttackAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MaxMagicAttack, GetMaxMagicAttackAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_MinMagicAttack, GetMinMagicAttackAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_Defense, GetDefenseAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);

	/** Resistance Attributes */
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Resistance_Fire, GetFireResistanceAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Resistance_Ice, GetIceResistanceAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Resistance_Lightning, GetLightningResistanceAttribute);
	AttributeTag_To_GetAttributeFuncPtr.Add(FMageGameplayTags::Instance().Attribute_Resistance_Physical, GetPhysicalResistanceAttribute);
	
#pragma endregion
}

void UMageAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	/**
	 * 只负责Clamp，不要再这写游戏逻辑 
	 * 可以响应 Setter 函数和 GameplayEffect 对 CurrentValue 的修改
	 * 预修改属性获取Clamp后的NewValue值，但这只发生在属性修改前，不会影响最后NewValue值（即NewValue值最终仍没有被Clamp）
	 * 
	 * 对于根据所有 Modifier 重新计算 CurrentValue 的函数需要在 PostGameplayEffectExecute 再次执行限制(Clamp)操作
	 */
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxHealth());
	}
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxMana());
	}
	if(Attribute == GetVitalityAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxVitality());
	}
	
}

void UMageAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	/** 存储Effect相关变量 */
	FEffectProperty Property;
	SetEffectProperty(Property, Data);

	/** 如果目标角色死亡, 则不再执行计算 */
	if(Property.TargetCharacter->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(Property.TargetCharacter)) return;
	
	/** 根据Primary Attribute 的变化更新 Secondary Attributes */
    if(Data.EvaluatedData.Attribute == GetStrengthAttribute())
	{
    	/** 更新相关的属性 */
		UpdateMaxHealth(Property.SourceCharacterClass, Property.SourceCharacterLevel);
    	// 保证先修改最大攻，再修改最小攻（匹配WBP_AttributeRow的更新逻辑）
    	UpdateMaxPhysicalAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
    	UpdateMinPhysicalAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
	}

	if(Data.EvaluatedData.Attribute == GetIntelligenceAttribute())
	{
		UpdateMaxMana(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateMaxMagicAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateMinMagicAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		
	}
	
	if(Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		UpdateMaxHealth(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateMaxVitality(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateDefense(Property.SourceCharacterClass, Property.SourceCharacterLevel);
	}
	
	if (Data.EvaluatedData.Attribute == GetVigorAttribute())
	{
		UpdateMaxPhysicalAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateMinPhysicalAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateMaxMagicAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateMinMagicAttack(Property.SourceCharacterClass, Property.SourceCharacterLevel);
		UpdateCriticalHitChance(Property.SourceCharacterClass, Property.SourceCharacterLevel);
	}

	/** Clamp 生命值 */
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		/* 再次Clamp */
		SetHealth(FMath::Clamp<float>(GetHealth(), 0.0f, GetMaxHealth()));
	}
	
	/** Clamp 法力值 */
	if(Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp<float>(GetMana(), 0.0f, GetMaxMana()));
	}

	/** Clamp 活力值 */
	if(Data.EvaluatedData.Attribute == GetVitalityAttribute())
	{
		SetVitality(FMath::Clamp<float>(GetVitality(), 0.0f, GetMaxVitality()));
	}


	/** 伤害计算, MetaAttribute不会被复制，所以以下只在服务器中进行 */
	if(Data.EvaluatedData.Attribute == GetMetaDamageAttribute())
	{
		CalcMetaDamage(Property);
	}

	/** 获取经验值计算, MetaAttribute不会被复制，所以以下只在服务器中进行 */
	if(Data.EvaluatedData.Attribute == GetMetaExpAttribute())
	{
		CalcMetaExp(Property);
	}
}

void UMageAttributeSet::CalcMetaDamage(const FEffectProperty& Property)
{
	const float TempMetaDamage = GetMetaDamage();
		
	if(TempMetaDamage > 0.0f)
	{
		const float NewHealth = GetHealth() - TempMetaDamage;
		SetHealth(FMath::Clamp<float>(NewHealth, 0.0f, GetMaxHealth()));
			
		const bool bIsDead = NewHealth <= 0.0f; // 用来判断死亡
		if(bIsDead)
		{
			/** 死亡反馈 */
			if(ICombatInterface* CombatInterface = Cast<ICombatInterface>(Property.TargetAvatarActor))
			{
				CombatInterface->Die(UMageAbilitySystemLibrary::GetDeathImpulse(Property.EffectContextHandle));
			}
			
			/** 发送经验值到Player */
			SendExpEvent(Property);
		}
		else
		{
			/** 受击反馈 */
			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FMageGameplayTags::Instance().Effects_HitReact);
			Property.TargetASC->TryActivateAbilitiesByTag(TagContainer); //激活 GA_HitReact, 注意要在GA_HitReact中添加Tag(Effects.HitReact)

			/** 击退 */
			FVector KnockbackForce = UMageAbilitySystemLibrary::GetKnockbackForce(Property.EffectContextHandle);
			if(!KnockbackForce.IsZero())
			{
				Property.TargetCharacter->LaunchCharacter(KnockbackForce, true, true); 
			}
		}

		/** 显示伤害浮动数字 */
		const bool bIsCriticalHit = UMageAbilitySystemLibrary::GetIsCriticalHit(Property.EffectContextHandle);
		ShowDamageFloatingText(Property, TempMetaDamage,bIsCriticalHit);

		/** Debuff */
		if(UMageAbilitySystemLibrary::GetIsDebuff(Property.EffectContextHandle))
		{
			Debuff(Property);
		}
	}
	else
	{
		//伤害为0仍显示
		ShowDamageFloatingText(Property, TempMetaDamage,false);
	}
		
	SetMetaDamage(0.0f); //元属性清0
}

void UMageAttributeSet::Debuff(const FEffectProperty& Property)
{
	/** 通过C++创建一个临时GE来实现Debuff */
	FMageGameplayTags MageGameplayTags = FMageGameplayTags::Instance();
	
	//创建新的EffectContext
	FGameplayEffectContextHandle EffectContextHandle = Property.SourceASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(Property.SourceAvatarActor);

	//获取Debuff信息
	const float DebuffDamage = UMageAbilitySystemLibrary::GetDebuffDamage(Property.EffectContextHandle);
	const float DebuffFrequency = UMageAbilitySystemLibrary::GetDebuffFrequency(Property.EffectContextHandle);
	const float DebuffDuration = UMageAbilitySystemLibrary::GetDebuffDuration(Property.EffectContextHandle);
	const FGameplayTag DamageTypeTag = UMageAbilitySystemLibrary::GetDamageTypeTag(Property.EffectContextHandle);

	//创建临时GE
	const FString DebuffName = FString::Printf(TEXT("DynamicDebuff_%s"), *DamageTypeTag.ToString());
	UGameplayEffect* DebuffEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(DebuffName));

	//设置DurationPolicy
	DebuffEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DebuffEffect->Period = DebuffFrequency;
	DebuffEffect->DurationMagnitude = FScalableFloat(DebuffDuration);

	//获取DebuffTag，该Tag会应用到目标Actor
	// - InheritableOwnedTagsContainer就是GrantedTags
	// - 在对应的拥有ASC的Actor类中中绑定 RegisterGameplayTagEvent 委托，监听Tag的变化
	const FGameplayTag DebuffTag = MageGameplayTags.DamageTypeTag_To_DebuffTag[DamageTypeTag];
	DebuffEffect->InheritableOwnedTagsContainer.AddTag(DebuffTag);

	//设置Stack
	DebuffEffect->StackingType = EGameplayEffectStackingType::AggregateBySource;
	DebuffEffect->StackLimitCount = 1;

	//设置Modifiers
	const int32 Index = DebuffEffect->Modifiers.Num(); //Modifiers索引
	DebuffEffect->Modifiers.Add(FGameplayModifierInfo());
	FGameplayModifierInfo& ModifierInfo = DebuffEffect->Modifiers[Index];
	ModifierInfo.ModifierMagnitude = FScalableFloat(DebuffDamage);
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.Attribute = GetMetaDamageAttribute();

	//应用GE
	if(const FGameplayEffectSpec* MutableSpec =new FGameplayEffectSpec(DebuffEffect, EffectContextHandle, 1.0f))
	{
		//注意这里GetContext返回的是新建的EffectContextHandle指向的EffectContext,而不是Property.EffectContextHandle
		FMageGameplayEffectContext* MageEffectContext = static_cast<FMageGameplayEffectContext*>(MutableSpec->GetContext().Get());
		MageEffectContext->SetDamageTypeTag(MakeShared<FGameplayTag>(DamageTypeTag));

		//应用到自身
		Property.TargetASC->ApplyGameplayEffectSpecToSelf(*MutableSpec);
	}
}

void UMageAttributeSet::CalcMetaExp(const FEffectProperty& Property)
{
	const float TempMetaExp = GetMetaExp();
		
		if(TempMetaExp>0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Silver, FString::Printf(TEXT("获得经验值：%f"), TempMetaExp));

			/**
			 * 增加PlayerState中的经验值，并广播经验值变化委托，该委托在OverlayWidgetController中被监听，用于更新经验条
			 * - 使用接口避免了直接访问PlayerState，进而防止PlayerState和属性集互相引用(即防止循环依赖，低耦合技巧）
			 * - SourceCharacter是Effect的发起者，Player 激活 GA_ListenForEvent 将 GE_EventBaseListen 应用到自身，所以发起者是Player
			 */
			if(IPlayerInterface* PlayerInterface = Cast<IPlayerInterface>(Property.SourceCharacter))
			{
				const int32 CurrentExp = PlayerInterface->GetExp();
				const int32 NewLevel = PlayerInterface->FindLevelForExp(CurrentExp + TempMetaExp); //计算新经验值后能升到几级
				PlayerInterface->AddToExp(TempMetaExp); //增加经验值
				
				//如果升级
				if(NewLevel > Property.SourceCharacterLevel)
				{
					//升级反馈
					PlayerInterface->LevelUp(); 
					
					//根据升级数计算奖励
					for(int i = Property.SourceCharacterLevel; i < NewLevel; ++i)
					{
						PlayerInterface->AddToLevel(1); //升级
						PlayerInterface->AddToAttributePoint(PlayerInterface->GetAttributePointReward(i)); //增加属性点
						PlayerInterface->AddToSkillPoint( PlayerInterface->GetSkillPointReward(i)); //增加技能点
					}
					
					// 更新等级相关的属性
					UpdateMaxHealth(Property.SourceCharacterClass, NewLevel);
					UpdateMaxMana(Property.SourceCharacterClass, NewLevel);
					UpdateMaxVitality(Property.SourceCharacterClass, NewLevel);
					// 保证先修改最大攻，再修改最小攻（匹配WBP_AttributeRow的更新逻辑）
					UpdateMaxPhysicalAttack(Property.SourceCharacterClass, NewLevel);
					UpdateMinPhysicalAttack(Property.SourceCharacterClass, NewLevel);
					UpdateMaxMagicAttack(Property.SourceCharacterClass, NewLevel);
					UpdateMinMagicAttack(Property.SourceCharacterClass, NewLevel);
					UpdateDefense(Property.SourceCharacterClass, NewLevel);

					//回满血\蓝\活力
					SetHealth(GetMaxHealth());
					SetMana(GetMaxMana());
					SetVitality(GetMaxVitality());

					//更新技能状态(等级达到技能要求时，将技能状态设置为可学习)
					if(UMageAbilitySystemComponent* MageASC = Cast<UMageAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
					{
						MageASC->UpdateAbilityState(NewLevel);
					}
				}
			}
		}
		SetMetaExp(0.0f); //元属性清0
}

void UMageAttributeSet::SetEffectProperty(FEffectProperty& Property, const FGameplayEffectModCallbackData& Data) const
{
	// Source是Effect的发起者，Target是Effect的目标(该AttributeSet的拥有者)
	Property.EffectContextHandle = Data.EffectSpec.GetContext();
	Property.SourceASC = Property.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if (IsValid(Property.SourceASC) && Property.SourceASC->AbilityActorInfo.IsValid())
	{
		Property.SourceAvatarActor = Property.SourceASC->GetAvatarActor();
		Property.SourceController = Property.SourceASC->AbilityActorInfo->PlayerController.Get();
		// 如果SourceController为空，尝试转为Pawn获取Controller
		if (Property.SourceController == nullptr && Property.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Property.SourceAvatarActor))
			{
				Property.SourceController = Pawn->GetController();
			}
		}

		if (Property.SourceController)
		{
			Property.SourceCharacter = Cast<ACharacter>(Property.SourceController->GetPawn());
			if(const ICombatInterface* CombatInterface = Cast<ICombatInterface>(Property.SourceCharacter))
			{
				Property.SourceCharacterLevel = CombatInterface->GetCharacterLevel();
				Property.SourceCharacterClass = CombatInterface->GetCharacterClass();
			}
		}
	}

	if (Data.Target.AbilityActorInfo.IsValid())
	{
		Property.TargetAvatarActor = Data.Target.GetAvatarActor();
		Property.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Property.TargetAvatarActor);
		Property.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Property.TargetCharacter = Cast<ACharacter>(Property.TargetAvatarActor);
		if(const ICombatInterface* CombatInterface = Cast<ICombatInterface>(Property.TargetCharacter))
		{
			Property.TargetCharacterLevel = CombatInterface->GetCharacterLevel();
			Property.TargetCharacterClass = CombatInterface->GetCharacterClass();
		}
	}
}

void UMageAttributeSet::SendExpEvent(const FEffectProperty& Property)
{
	if(const ICombatInterface* CombatInterface = Cast<ICombatInterface>(Property.TargetCharacter))
	{
		const int32 TargetLevel = CombatInterface->GetCharacterLevel();
		const ECharacterClass TargetClass = CombatInterface->GetCharacterClass();

		//根据 Enemy 职业和等级获取经验值
		const int32 ExpReward = UMageAbilitySystemLibrary::GetExpRewardForClassAndLevel(Property.TargetCharacter, TargetClass, TargetLevel);

		//自定义Payload
		FGameplayEventData Payload;
		Payload.EventTag = FMageGameplayTags::Instance().Attribute_Meta_Exp;
		Payload.EventMagnitude = ExpReward;

		//发送事件到 Player, 该事件在GA_ListenForEvent蓝图中被监听
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Property.SourceCharacter, FMageGameplayTags::Instance().Attribute_Meta_Exp,Payload);
	}
}

// TODO：为每个职业设计不同的属性加成
#pragma region 更新 Secondary Attributes
void UMageAttributeSet::UpdateMaxHealth(ECharacterClass CharacterClass, float CharacterLevel)
{
	// if(CharacterClass == ECharacterClass::Mage)
	// {
	//	...
	// }
	// else if(CharacterClass == ECharacterClass::Warrior)
	// ...
	const float NewMaxHealth = GetStrength() * 2.0f + GetStamina() * 19.4f + CharacterLevel * 10.0f;
	SetMaxHealth(NewMaxHealth);
}

void UMageAttributeSet::UpdateMaxMana(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewMaxMana = GetIntelligence() * 3.0f + CharacterLevel * 15.0f;
	SetMaxMana(NewMaxMana);
}

void UMageAttributeSet::UpdateMaxVitality(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewMaxVitality = 100.0f + GetStamina() * 0.1f + CharacterLevel * 1.0f;
	SetMaxVitality(NewMaxVitality);
}

void UMageAttributeSet::UpdateMinPhysicalAttack(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewMinPhysicalAttack = GetStrength() * 1.3f + GetVigor() * 1.7f + CharacterLevel;
	SetMinPhysicalAttack(NewMinPhysicalAttack);
}

void UMageAttributeSet::UpdateMaxPhysicalAttack(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewMaxPhysicalAttack = GetStrength() * 1.3f + GetVigor() * 2.5f + CharacterLevel;
	SetMaxPhysicalAttack(NewMaxPhysicalAttack);
}

void UMageAttributeSet::UpdateMinMagicAttack(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewMinMagicAttack = GetIntelligence() * 2.2f + GetVigor() * 1.7f + CharacterLevel;
	SetMinMagicAttack(NewMinMagicAttack);
}

void UMageAttributeSet::UpdateMaxMagicAttack(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewMaxMagicAttack = GetIntelligence() * 2.2f + GetVigor() * 2.5f + CharacterLevel;
	SetMaxMagicAttack(NewMaxMagicAttack);
}

void UMageAttributeSet::UpdateDefense(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewDefense = GetStamina() * 4.8f + CharacterLevel;
	SetDefense(NewDefense);
}

void UMageAttributeSet::UpdateCriticalHitChance(ECharacterClass CharacterClass, float CharacterLevel)
{
	const float NewCriticalHitChance = 0.1 +  GetVigor() * 0.01f;
	SetCriticalHitChance(NewCriticalHitChance);
}

#pragma endregion

void UMageAttributeSet::ShowDamageFloatingText(const FEffectProperty& Property, const float DamageValue, const bool bIsCriticalHit) const
{
	//在伤害计算中被调用，因此只在服务器中调用
	if(Property.SourceCharacter!=Property.TargetCharacter)
	{
		//当玩家攻击敌人时, 玩家是Source
		if (AMagePlayerController* SourcePC = Cast<AMagePlayerController>(Property.SourceController)) 
		{
			SourcePC->AttachDamageFloatingTextToTarget(DamageValue, Property.TargetCharacter, bIsCriticalHit);
		}
		//当敌人攻击玩家时，玩家是Target
		else if(AMagePlayerController* TargetPC = Cast<AMagePlayerController>(Property.TargetController))
		{
			TargetPC->AttachDamageFloatingTextToTarget(DamageValue, Property.TargetCharacter, bIsCriticalHit);
		}
	}
}

