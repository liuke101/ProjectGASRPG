#include "GAS/MageAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UMageAttributeSet::UMageAttributeSet()
{
	/* 初始化Base/Current Value */
	InitHealth(50.0f);
	InitMaxHealth(100.0f);
	InitMana(50.0f);
	InitMaxMana(100.0f);
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
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMageAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
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
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 9999.0f);
	}
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, GetMaxMana());
	}
	if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Clamp<float>(NewValue, 0.0f, 9999.0f);
	}
}

void UMageAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FEffectProperty Property;
	SetEffectProperty(Property, Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		/* 再次Clamp */
		SetHealth(FMath::Clamp<float>(GetHealth(), 0.0f, GetMaxHealth()));
		SetMaxHealth(FMath::Clamp<float>(GetMaxHealth(), 0.0f, 9999.0f));
		SetMana(FMath::Clamp<float>(GetMana(), 0.0f, GetMaxMana()));
		SetMaxMana(FMath::Clamp<float>(GetMaxMana(), 0.0f, 9999.0f));
	}
}

void UMageAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	/* 该宏用于 RepNotify 函数，以处理将被客户端预测修改的属性。 */
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Health, OldHealth);
}

void UMageAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MaxHealth, OldMaxHealth);
}

void UMageAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, Mana, OldMana);
}

void UMageAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMageAttributeSet, MaxMana, OldMaxMana)
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
