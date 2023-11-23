#include "GAS/GameplayEffect/MageEffectActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Component/GameplayTagsComponent.h"
#include "GAS/MageAbilitySystemLibrary.h"
#include "GAS/MageGameplayTags.h"

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
	if(bIgnoreActor(TargetActor)) return;
	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!IsValid(TargetASC)) return;
	
	checkf(GameplayEffectClass, TEXT("GameplayEffectClass is nullptr"));

	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(
		GameplayEffectClass, EffectLevel, EffectContextHandle); //设置了Effect等级

	/* 应用GameplayEffectSpec */
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data); //注意第一个参数传的是引用类型，需要*解引用

	/* 获取对应的GameplayEffect用于判断 */
	const UGameplayEffect* GameplayEffect = EffectSpecHandle.Data->Def.Get();

	/* 处理 Instant Effect */
	if(GameplayEffect->DurationPolicy == EGameplayEffectDurationType::Instant &&
		bDestroyEffectAfterApplication)
	{
		// 如果标记bDestroyEffectAfterApplication为true，则在应用后销毁(Instant)Effect
		Destroy();
	}

	/* 处理 Duration Effect */
	if(GameplayEffect->DurationPolicy == EGameplayEffectDurationType::HasDuration &&
		bDestroyEffectAfterApplication)
	{
		Destroy();
	}
	
	/* 处理 Infinite Effect */
	if(GameplayEffect->DurationPolicy == EGameplayEffectDurationType::Infinite &&
		InfiniteEffectRemovalPolicy == EffectRemovalPolicy::ERP_RemoveOnEndOverlap)
	{
		//用Map保存ActiveEffectHandle和对应的TargetActor的ASC，防止应用多个Infinite Effect时覆盖掉之前的Handle
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC); 
	}

	
}

void AMageEffectActor::OnEffectBeginOverlap(AActor* TargetActor)
{
	if(bIgnoreActor(TargetActor)) return;
	
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
	if(bIgnoreActor(TargetActor)) return;
	
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

bool AMageEffectActor::bIgnoreActor(const AActor* TargetActor) const
{
	// 如果目标是含有Tag，且不允许对玩家/敌人应用效果，则忽略
	if(const IGameplayTagAssetInterface* TagAssetInterface = Cast<IGameplayTagAssetInterface>(TargetActor))
	{
		// 该 Pawn 的 Tag 如果是 Character_Enemy, 则 TargetTag 为 Character_Player
		const FMageGameplayTags& GameplayTags = FMageGameplayTags::Get();
		
		if(TagAssetInterface->HasMatchingGameplayTag(GameplayTags.Character_Player) && !bApplyEffectsToPlayer)
		{
			return true;
		}

		if(TagAssetInterface->HasMatchingGameplayTag(GameplayTags.Character_Enemy) && !bApplyEffectsToEnemy)
		{
			return true;
		}
	}
	return false;

}
