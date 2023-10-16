#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interface/CombatInterface.h"
#include "MageCharacterBase.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UAbilitySystemComponent;

UCLASS(Abstract)
class PROJECTGASRPG_API AMageCharacterBase : public ACharacter, public IAbilitySystemInterface,public ICombatInterface
{
	GENERATED_BODY()

public:
	AMageCharacterBase();

protected:
	virtual void BeginPlay() override;

#pragma region Weapon
protected:
	UPROPERTY(EditAnywhere, Category = "Mage_Weapon")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	UPROPERTY(EditAnywhere, Category = "Mage_Weapon")
	FName WeaponTipSocketName; // 武器顶端Socket

	virtual FVector GetWeaponSocketLocation() override;
#pragma endregion
	
#pragma region GAS
	/**
	 * MageCharacter类需要持久化 Attribute 数据，故采用 OwnerActor 和 AvatarActor 分离的方式：
	 * MagePlayerState 为 OwnerActor，MageCharacterBase 为AvatarActor
	 * 
	 * MageEnemy类采用 OwnerActor 和 AvatarActor 合一的方式：
	 * OwnerActor, AvatarActor 都是自身
	 */

	/**
	 * OwnerActor 和 AvatarActor 都需要继承并实现 IAbilitySystemInterface
	 * 我们在基类中继承了 IAbilitySystemInterface（子类 MageCharacter，MageEnemy也完成了继承）, 我们只需要再在 MagePlayerState 中继承即可。
	 *
	 * 继承后需要实现GetAbilitySystemComponent()方法
	 */
	
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; };
	FORCEINLINE virtual UAttributeSet* GetAttributeSet() const { return AttributeSet; };
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	virtual void InitAbilityActorInfo();

protected:
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultVitalAttribute;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttribute;

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultSecondaryAttribute;

	
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass,float Level) const;
	
	/* 使用GameplayEffect初始化主要属性 */
	UFUNCTION()
	virtual void InitDefaultAttributes() const;
	
	/**
	 * 向ASC授予所有GameplayAbility，将 GA 的 Tag 添加到AbilitySpec，这些 Tag 将于输入的 Tag 进行匹配
	 *
	 * 对于拥有 PlayerController 的 Character，在 PossessedBy() 中调用
	 */
	void AddCharacterAbilities() const;
	
private:
	UPROPERTY(EditAnywhere, Category = "Mage_GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
#pragma endregion
};
