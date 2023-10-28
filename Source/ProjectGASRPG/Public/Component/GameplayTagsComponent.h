#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GameplayTagsComponent.generated.h"


 UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
 class PROJECTGASRPG_API UGameplayTagsComponent : public UActorComponent
 {
 	GENERATED_BODY()

 public:
 	UGameplayTagsComponent();

 protected:
 	virtual void BeginPlay() override;

 public:
 	FORCEINLINE const FGameplayTagContainer& GetGameplayTags() const {return GameplayTags;}
 	
 private:
 	UPROPERTY(EditAnywhere, Category = "Mage_GameplayTag")
 	FGameplayTagContainer GameplayTags = FGameplayTagContainer();
 };
