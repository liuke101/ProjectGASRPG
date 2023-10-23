#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "MageEnemy.generated.h"

enum class ECharacterClass : uint8;
class UWidgetComponent;

UCLASS()
class PROJECTGASRPG_API AMageEnemy : public AMageCharacterBase, public IEnemyInterface
{
	GENERATED_BODY()

public:
	AMageEnemy();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region EnemyInterface
public:
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
#pragma endregion

#pragma region CombatInterface
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_GAS")
	float LifeSpan = 5.0f;
public:
	FORCEINLINE virtual int32 GetCharacterLevel() const override { return Level; }

	virtual void Die() override;
#pragma endregion
	
#pragma region GAS
public: 
	virtual void InitAbilityActorInfo() override;

protected:
	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UPROPERTY(BlueprintReadOnly, Category = "Mage_GAS")
	bool bHitReacting;

	float BaseWalkSpeed = 250.0f;
#pragma endregion

#pragma region UI
	/** Enemy本身作为WidgetController，广播数据到HealthBar */
public:
	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Mage_Delegates")
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mage_UI")
	TObjectPtr<UWidgetComponent> HealthBar;

	
#pragma endregion
};
