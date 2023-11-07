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
private:



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
	/** 初始化默认属性 */
	virtual void InitDefaultAttributes() const override;

	/**
	 * 向ASC授予（Give）所有CharacterAbilities，将 GA 的 Tag 添加到AbilitySpec，这些 Tag 将于输入的 Tag 进行匹配
	 * - 对于拥有 PlayerController 的 Character，在 PossessedBy() 中调用
	 */
	void GivePlayerAbilities() const;

private:
	UPROPERTY(EditAnywhere, Category = "Mage_GAS")
	TArray<TSubclassOf<UGameplayAbility>> PlayerAbilities;
	
	/** 初始化 */
	virtual void InitAbilityActorInfo() override;

	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultVitalAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultResistanceAttribute;
	
#pragma endregion
	
#pragma region CombatInterface
public:
	virtual  int32 GetCharacterLevel() const override;
	
#pragma endregion

public:
	virtual ECharacterClass GetCharacterClass() const override;
	
};
