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
	/* 获取目标Actor的 AbilitySystemComponent */
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC == nullptr) return;
	checkf(GameplayEffectClass, TEXT("GameplayEffectClass为空"));
	
	/* 创建GameplayEffectContextHandle */
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	/* EffectSpecHandle允许蓝图蓝图生成一个 GameplayEffectSpec，然后通过该句柄的共享指针 Data 引用它，以便多次应用/应用多个目标 */
	FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, 1.0f, EffectContextHandle);

	/* 应用GameplayEffectSpec */
	TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get()); //注意第一个参数传的是引用类型，Get获取原始指针后还需要*解引用
}
