// 


#include "Component/DebuffNiagaraComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Interface/CombatInterface.h"

UDebuffNiagaraComponent::UDebuffNiagaraComponent()
{
	bAutoActivate = false;
}

void UDebuffNiagaraComponent::BeginPlay()
{
	Super::BeginPlay();

	ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetOwner());
	
	//绑定FOnGameplayEffectTagCountChanged委托，当DebuffTag的数量发生变化时，调用DebuffChanged回调
	if(UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		ASC->RegisterGameplayTagEvent(DebuffTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UDebuffNiagaraComponent::DebuffChangedCallback);
	}
	// 如果Character此时还没有注册ASC, 则绑定OnASCRegistered委托
	// 当Character的ASC注册完成时广播该委托，组件就知道此时有ASC了，进而完成绑定FOnGameplayEffectTagCountChanged委托
	else if(CombatInterface)
	{
		CombatInterface->GetOnASCRegisteredDelegate().AddWeakLambda(this, [this](UAbilitySystemComponent* InASC)
		{
			InASC->RegisterGameplayTagEvent(DebuffTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UDebuffNiagaraComponent::DebuffChangedCallback);
		});
	}

	// 绑定OnDeath委托，当Character死亡时，调用OnOwnerDeath回调
	if(CombatInterface)
	{
		CombatInterface->GetOnDeathDelegate().AddDynamic(this,&UDebuffNiagaraComponent::OnOwnerDeathCallback);
	}
	
}

void UDebuffNiagaraComponent::DebuffChangedCallback(const FGameplayTag CallbackTag, int32 NewCount)
{
	const bool bIsDead = GetOwner() && GetOwner()->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(GetOwner());
	
	if(NewCount > 0 && !bIsDead) 
	{
		Activate();
	}
	else
	{
		Deactivate();
	}
}

void UDebuffNiagaraComponent::OnOwnerDeathCallback(AActor* DeadActor)
{
	Deactivate();
}
