#include "GAS/MageAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"
#include "Interface/CombatInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/MagePlayerController.h"

UMageAttributeSet::UMageAttributeSet()
{
	/** 初始化Map */
	/** Vital Attributes */
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Vital_Health, GetHealthAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Vital_Mana, GetManaAttribute);

	/** Primary Attributes */
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Primary_Strength, GetStrengthAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Primary_Intelligence, GetIntelligenceAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Primary_Stamina, GetStaminaAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Primary_Vigor, GetVigorAttribute);

	/** Secondary Attributes */
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_MaxHealth, GetMaxHealthAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_MaxMana, GetMaxManaAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_MaxPhysicalAttack, GetMaxPhysicalAttackAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_MinPhysicalAttack, GetMinPhysicalAttackAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_MaxMagicAttack, GetMaxMagicAttackAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_MinMagicAttack, GetMinMagicAttackAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_Defense, GetDefenseAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);

	/** Resistance Attributes */
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Resistance_Fire, GetFireResistanceAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Resistance_Ice, GetIceResistanceAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Resistance_Lightning, GetLightningResistanceAttribute);
	TagsToAttributes.Add(FMageGameplayTags::Get().Attribute_Resistance_Physical, GetPhysicalResistanceAttribute);
}

void UMageAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/*
	 * 这里列出我们想要复制的属性
	 * 
	 * COND_None：此属性没有条件，并将在其发生变化时随时发送（服务器）
	 * REPNOTIFY_Always：RepNotify函数在客户端值已经与服务端复制的值相同的情况下也会触发(因为有预测)
	 */
	
	/* Vital Attributes */
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	
	/* Primary Attributes */
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Vigor, COND_None, REPNOTIFY_Always);

	/* Secondary Attributes */
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MaxPhysicalAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MinPhysicalAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MaxMagicAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MinMagicAttack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);

	/** Resistance Attributes */
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, IceResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, LightningResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
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
	
	
}

void UMageAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	/** 存储Effect相关变量 */
	FEffectProperty Property;
	SetEffectProperty(Property, Data);

	/** Clamp */
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		/* 再次Clamp */
		SetHealth(FMath::Clamp<float>(GetHealth(), 0.0f, GetMaxHealth()));
	}

	if(Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp<float>(GetMana(), 0.0f, GetMaxMana()));
	}

	/** 伤害计算, MetaAttribute不会被赋值，所以以下只在服务器中进行 */
	if(Data.EvaluatedData.Attribute == GetMetaDamageAttribute())
	{
		const float TempMetaDamage = GetMetaDamage();
		
		if(TempMetaDamage > 0.0f)
		{
			const float NewHealth = GetHealth() - TempMetaDamage;
			SetHealth(FMath::Clamp<float>(NewHealth, 0.0f, GetMaxHealth()));
			
			const bool bIsDead = NewHealth <= 0.0f; // 用来判断死亡
			if(bIsDead)
			{
				if(ICombatInterface* CombatInterface = Cast<ICombatInterface>(Property.TargetAvatarActor))
				{
					CombatInterface->Die();
				}
			}
			else
			{
				/** 受击反馈 */
				FGameplayTagContainer TagContainer;
				TagContainer.AddTag(FMageGameplayTags::Get().Effects_HitReact);
				Property.TargetASC->TryActivateAbilitiesByTag(TagContainer); //激活 GA_HitReact, 注意要在GA_HitReact中添加Tag(Effects.HitReact)
			}

			/** 显示伤害浮动数字 */
			const bool bIsCriticalHit = UMageAbilitySystemLibrary::GetIsCriticalHit(Property.EffectContextHandle);
			ShowDamageFloatingText(Property, TempMetaDamage,bIsCriticalHit);
		}
		else
		{
			//伤害为0仍显示
			ShowDamageFloatingText(Property, TempMetaDamage,false);
		}
		

		SetMetaDamage(0.0f); //清0
		
	}
}

/* GAMEPLAYATTRIBUTE_REPNOTIFY() 宏用于 RepNotify 函数，以处理将被客户端预测修改的属性。 */
void UMageAttributeSet::OnRep_Health(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Health, OldData);
}

void UMageAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Mana, OldData);
}

void UMageAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Strength, OldData);
}

void UMageAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Intelligence, OldIntelligence);
}

void UMageAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Stamina, OldData);
}

void UMageAttributeSet::OnRep_Vigor(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Vigor, OldData);
}

void UMageAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MaxHealth, OldData);
}

void UMageAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MaxMana, OldData)
}

void UMageAttributeSet::OnRep_MaxPhysicalAttack(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MaxPhysicalAttack, OldData);
}

void UMageAttributeSet::OnRep_MinPhysicalAttack(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MinPhysicalAttack, OldData);
}

void UMageAttributeSet::OnRep_MaxMagicAttack(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MaxMagicAttack, OldData);
}

void UMageAttributeSet::OnRep_MinMagicAttack(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MinMagicAttack, OldData);
}

void UMageAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Defense, OldData);
}

void UMageAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, CriticalHitChance, OldData);
}

void UMageAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, FireResistance, OldData);
}

void UMageAttributeSet::OnRep_IceResistance(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, IceResistance, OldData);
}

void UMageAttributeSet::OnRep_LightningResistance(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, LightningResistance, OldData);
}

void UMageAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldData) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, PhysicalResistance, OldData);
}

void UMageAttributeSet::SetEffectProperty(FEffectProperty& Property,
                                          const FGameplayEffectModCallbackData& Data) const
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
		}
	}

	if (Data.Target.AbilityActorInfo.IsValid())
	{
		Property.TargetAvatarActor = Data.Target.GetAvatarActor();
		Property.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Property.TargetAvatarActor);

		Property.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		if (Property.TargetController == nullptr && Property.TargetAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Property.TargetAvatarActor))
			{
				Property.TargetController = Pawn->GetController();
			}
		}

		if (Property.TargetController)
		{
			Property.TargetCharacter = Cast<ACharacter>(Property.TargetController->GetPawn());
		}
	}
}

void UMageAttributeSet::ShowDamageFloatingText(const FEffectProperty& Property, const float DamageValue,
	const bool bIsCriticalHit) const
{
	// 在伤害计算中被调用，因此只在服务器中调用，AttachDamageFloatingTextToTarget是Client RPC, 这样就可以在客户端执行
	if(Property.SourceCharacter!=Property.TargetCharacter)
	{
		if (AMagePlayerController* PC = Cast<AMagePlayerController>(Property.SourceCharacter->Controller)) 
		{
			PC->AttachDamageFloatingTextToTarget(DamageValue, Property.TargetCharacter, bIsCriticalHit);
		}
	}
}

