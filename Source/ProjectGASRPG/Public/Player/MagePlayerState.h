#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "MagePlayerState.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;

UCLASS()
class PROJECTGASRPG_API AMagePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AMagePlayerState();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
#pragma region GAS
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override {return AbilitySystemComponent;}
	FORCEINLINE virtual UAttributeSet* GetAttributeSet() const {return AttributeSet;}
	FORCEINLINE int32 GetPlayerLevel() const {return Level;}
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	TObjectPtr<UAttributeSet> AttributeSet;

private:
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Level, Category = "Mage_GAS")
	int32 Level = 1;
	
	UFUNCTION()
	virtual void OnRep_Level(int32 OldLevel);

#pragma endregion
};
