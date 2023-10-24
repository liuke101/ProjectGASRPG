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
	int32 AbilityLevel = 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Input")
	FGameplayTag StartupInputTag;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Mage_Modifier")
	FScalableFloat Damage;  //使用曲线表格控制技能伤害（随等级变化）

	
};
