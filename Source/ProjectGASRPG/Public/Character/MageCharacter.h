#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "MageCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class PROJECTGASRPG_API AMageCharacter : public AMageCharacterBase
{
	GENERATED_BODY()

public:
	AMageCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region Camera
public:
	FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm; }
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera",meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera",meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
#pragma endregion
	
#pragma region GAS
public:
	/** 服务器初始化  */
	virtual void PossessedBy(AController* NewController) override;
	
	/** 客户端初始化, 在 PlayerState 复制到客户端时进行回调*/
	virtual void OnRep_PlayerState() override;
	
protected:
	virtual void InitDefaultAttributes() const override;
	
private:
	/** 初始化 */
	virtual void InitAbilityActorInfo() override;

	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultVitalAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultSecondaryAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultResistanceAttribute;
	
#pragma endregion
	
#pragma region CombatInterface
public:
	virtual  int32 GetCharacterLevel() const override;
	
#pragma endregion
};
