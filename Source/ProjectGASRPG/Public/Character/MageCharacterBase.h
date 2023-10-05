#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MageCharacterBase.generated.h"

UCLASS(Abstract)
class PROJECTGASRPG_API AMageCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AMageCharacterBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere,Category="RPG_Combat")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
};
