#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MageGameplayAbility.generated.h"


UCLASS()
class PROJECTGASRPG_API UMageGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UMageGameplayAbility();

public:
	UPROPERTY(EditAnywhere, Category = "Mage_GA")
	int32 StartupAbilityLevel = 1; //技能初始等级(默认为1即可)
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Input")
	FGameplayTag StartupInputTag;

	virtual FString GetDescription(int32 AbilityLevel);
	virtual FString GetNextLevelDescription(int32 AbilityLevel);
	static FString GetLockedDescription(int32 LevelRequirement);
};
