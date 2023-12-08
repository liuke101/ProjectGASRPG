// 


#include "GAS/AsyncTask/AsyncTask_AttributeChanged.h"

#include "AbilitySystemComponent.h"

UAsyncTask_AttributeChanged* UAsyncTask_AttributeChanged::ListenForAttributeChange(
	UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	UAsyncTask_AttributeChanged* AsyncTaskAttributeChanged = NewObject<UAsyncTask_AttributeChanged>();
	AsyncTaskAttributeChanged->ASC = AbilitySystemComponent;
	AsyncTaskAttributeChanged->Attribute = Attribute;

	if(!IsValid(AbilitySystemComponent) || !Attribute.IsValid())
	{
		AsyncTaskAttributeChanged->EndTask();
		return nullptr;
	}

	// 绑定属性变化委托
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(AsyncTaskAttributeChanged, &UAsyncTask_AttributeChanged::AttributeChangedCallback);
	
	return AsyncTaskAttributeChanged;
}

UAsyncTask_AttributeChanged* UAsyncTask_AttributeChanged::ListenForAttributesChange(
	UAbilitySystemComponent* AbilitySystemComponent, TArray<FGameplayAttribute> Attributes)
{
	UAsyncTask_AttributeChanged* AsyncTaskAttributeChanged = NewObject<UAsyncTask_AttributeChanged>();
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
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GameplayAttribute).AddUObject(AsyncTaskAttributeChanged, &UAsyncTask_AttributeChanged::AttributeChangedCallback);
	}
	
	return AsyncTaskAttributeChanged;
}

void UAsyncTask_AttributeChanged::EndTask()
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

void UAsyncTask_AttributeChanged::AttributeChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnAttributeChanged.Broadcast(Data.Attribute, Data.NewValue, Data.OldValue);
}
