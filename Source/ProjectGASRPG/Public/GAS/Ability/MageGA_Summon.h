﻿#pragma once

#include "CoreMinimal.h"
#include "MageGameplayAbility.h"
#include "MageGA_Summon.generated.h"

/** 召唤技能 */
UCLASS()
class PROJECTGASRPG_API UMageGA_Summon : public UMageGameplayAbility
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Mage_GA|SummonAbility")
	TArray<FVector> GetSpawnLocations() const;

	UFUNCTION(BlueprintPure, Category = "Mage_GA|SummonAbility")
	TSubclassOf<APawn> GetRandomSummonClass() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Summon")
	TArray<TSubclassOf<APawn>> SummonClasses;
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Summon")
	int32 SummonCount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_Summon")
	float MinSpawnDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_Summon")
	float MaxSpawnDistance = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Mage_Summon")
	float SpawnSpread = 90.0f;
};
