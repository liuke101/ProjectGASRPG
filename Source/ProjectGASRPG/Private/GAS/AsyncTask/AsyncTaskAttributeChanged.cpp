// 


#include "GAS/AsyncTask/AsyncTaskAttributeChanged.h"

#include "AbilitySystemComponent.h"

UAsyncTaskAttributeChanged* UAsyncTaskAttributeChanged::ListenForAttributeChange(
	UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	UAsyncTaskAttributeChanged* AsyncTaskAttributeChanged = NewObject<UAsyncTaskAttributeChanged>();
	AsyncTaskAttributeChanged->ASC = AbilitySystemComponent;
	AsyncTaskAttributeChanged->Attribute = Attribute;

	if(!IsValid(AbilitySystemComponent) || !Attribute.IsValid())
	{
		AsyncTaskAttributeChanged->EndTask();
		return nullptr;
	}

	// 绑定属性变化委托
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(AsyncTaskAttributeChanged, &UAsyncTaskAttributeChanged::AttributeChangedCallback);

	return AsyncTaskAttributeChanged;
}

UAsyncTaskAttributeChanged* UAsyncTaskAttributeChanged::ListenForAttributesChange(
	UAbilitySystemComponent* AbilitySystemComponent, TArray<FGameplayAttribute> Attributes)
{
	UAsyncTaskAttributeChanged* AsyncTaskAttributeChanged = NewObject<UAsyncTaskAttributeChanged>();
	AsyncTaskAttributeChanged->ASC = AbilitySystemComponent;
	AsyncTaskAttributeChanged->Attributes = Attributes;

	if(!IsValid(AbilitySystemComponent) || Attributes.IsEmpty())
	{
		AsyncTaskAttributeChanged->EndTask();
		return nullptr;
	}

	// 绑定属性变化委托
	for(const FGameplayAttribute GameplayAttribute : Attributes)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GameplayAttribute).AddUObject(AsyncTaskAttributeChanged, &UAsyncTaskAttributeChanged::AttributeChangedCallback);
	}
	
	return AsyncTaskAttributeChanged;
}

void UAsyncTaskAttributeChanged::EndTask()
{
	if (IsValid(ASC))
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);

		for (const FGameplayAttribute GameplayAttribute : Attributes)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(GameplayAttribute).RemoveAll(this);
		}
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTaskAttributeChanged::AttributeChangedCallback(const FOnAttributeChangeData& Data) const
{
	// 广播时，pin就会激活？
	// OnAttributeChanged连线就相当于绑定了该委托？
	OnAttributeChanged.Broadcast(Data.Attribute, Data.NewValue, Data.OldValue);
}
