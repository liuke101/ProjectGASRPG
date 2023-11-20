#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MageGameMode.generated.h"

class UAbilityDataAsset;
class UCharacterClassDataAsset;

UCLASS()
class PROJECTGASRPG_API AMageGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Maga_GameMode")
	TObjectPtr<UCharacterClassDataAsset> CharacterClassDataAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Maga_GameMode")
	TObjectPtr<UAbilityDataAsset> AbilityDataAsset;
};
