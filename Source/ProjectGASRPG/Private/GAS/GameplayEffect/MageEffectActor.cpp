#include "GAS/GameplayEffect/MageEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

AMageEffectActor::AMageEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);
}


void AMageEffectActor::BeginPlay()
{
	Super::BeginPlay();
}

void AMageEffectActor::ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	/** 获取目标Actor的 AbilitySystemComponent */
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC)) return;
	checkf(GameplayEffectClass, TEXT("GameplayEffectClass is nullptr"));

	/* 创建GameplayEffectContextHandle */
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	/*
	 * EffectSpecHandle允许蓝图生成一个 GameplayEffectSpec，然后通过该句柄的共享指针 Data 引用它，以便多次应用/应用多个目标
	 * 这里还设置了Effect等级
	 */
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(
		GameplayEffectClass, EffectLevel, EffectContextHandle);

	/* 应用GameplayEffectSpec */
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get()); //注意第一个参数传的是引用类型，Get获取原始指针后还需要*解引用
	
	/* 处理Infinite Effect */
	const bool bIsInfinite =  EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	if(bIsInfinite && InfiniteEffectRemovalPolicy == EffectRemovalPolicy::ERP_RemoveOnEndOverlap)
	{
		//用Map保存ActiveEffectHandle和对应的TargetActor的ASC，防止应用多个Infinite Effect时覆盖掉之前的Handle
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC); 
	}
}

void AMageEffectActor::OnEffectBeginOverlap(AActor* TargetActor)
{
	if(IsValid(InstantGameplayEffectClass) && InstantEffectApplicationPolicy == EffectApplicationPolicy::EAP_ApplyOnBeginOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if(IsValid(DurationGameplayEffectClass) && DurationEffectApplicationPolicy == EffectApplicationPolicy::EAP_ApplyOnBeginOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if(IsValid(InfiniteGameplayEffectClass) && InfiniteEffectApplicationPolicy == EffectApplicationPolicy::EAP_ApplyOnBeginOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AMageEffectActor::OnEffectEndOverlap(AActor* TargetActor)
{
	if (IsValid(InstantGameplayEffectClass) && InstantEffectApplicationPolicy == EffectApplicationPolicy::EAP_ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	
	if (IsValid(DurationGameplayEffectClass) && DurationEffectApplicationPolicy == EffectApplicationPolicy::EAP_ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	
	if(IsValid(InfiniteGameplayEffectClass))
	{
		if(InfiniteEffectApplicationPolicy == EffectApplicationPolicy::EAP_ApplyOnEndOverlap)
		{
			ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
		}
		
		if(InfiniteEffectRemovalPolicy == EffectRemovalPolicy::ERP_RemoveOnEndOverlap)
		{
			/* 从Map中移除对应的Handle和对应的Effect */
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
			if (!IsValid(TargetASC)) return;
		
			//循环遍历map容器时，不建议直接删除元素，因为这可能会导致迭代器失效，引发未定义的行为(其他容器也要考虑这个情况)
			//如果需要删除元素，可以先将需要删除的元素的Key值用TArray标记起来，然后在循环结束后再进行删除操作。
			TArray<FActiveGameplayEffectHandle> KeysToDelete;
			for (auto& Elem : ActiveEffectHandles)
			{
				if (Elem.Value == TargetASC)
				{
					TargetASC->RemoveActiveGameplayEffect(Elem.Key);
					KeysToDelete.Add(Elem.Key);
					break;
				}
			}

			for(auto& Key : KeysToDelete)
			{
				ActiveEffectHandles.Remove(Key);
			}
		}
	}
	

		
	
}
