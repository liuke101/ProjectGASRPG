// 


#include "Component/GameplayTagsComponent.h"


UGameplayTagsComponent::UGameplayTagsComponent()
{
}

void UGameplayTagsComponent::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = GameplayTags;
}


void UGameplayTagsComponent::BeginPlay()
{
	Super::BeginPlay();
}
