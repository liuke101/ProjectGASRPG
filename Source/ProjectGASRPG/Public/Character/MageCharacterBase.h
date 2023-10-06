﻿#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MageCharacterBase.generated.h"

class UAttributeSet;
class UAbilitySystemComponent;

UCLASS(Abstract)
class PROJECTGASRPG_API AMageCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMageCharacterBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Mage_Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

#pragma region GAS
public:
	/**
	 * MageCharacter类需要持久化 Attribute 数据，故采用 OwnerActor 和 AvatarActor 分离的方式：
	 * MagePlayerState 为 OwnerActor，MageCharacterBase 为AvatarActor
	 * 
	 * MageEnemy类采用 OwnerActor 和 AvatarActor 合一的方式：
	 * OwnerActor, AvatarActor 都是自身
	 */

	/**
	 *OwnerActor 和 AvatarActor 都需要继承并实现 IAbilitySystemInterface
	 *我们在基类中继承了 IAbilitySystemInterface（子类 MageCharacter，MageEnemy也完成了继承）, 我们只需要再在 MagePlayerState 中继承即可。
	 *
	 *继承后需要实现GetAbilitySystemComponent()方法
	 */
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
#pragma endregion
};
