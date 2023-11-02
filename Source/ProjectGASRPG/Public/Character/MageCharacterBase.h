#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/CombatInterface.h"
#include "MageCharacterBase.generated.h"

class UGameplayTagsComponent;
struct FGameplayTagContainer;
class UGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UAbilitySystemComponent;

UCLASS(Abstract)
class PROJECTGASRPG_API AMageCharacterBase : public ACharacter, public IAbilitySystemInterface,public IGameplayTagAssetInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	AMageCharacterBase();

protected:
	virtual void BeginPlay() override;

#pragma region Weapon
protected:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Mage_Weapon")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	UPROPERTY(EditAnywhere, Category = "Mage_Weapon")
	FName WeaponTipSocketName; // 武器顶端Socket
#pragma endregion

#pragma region ICombatInterface
public:
	virtual UAnimMontage* GetHitReactMontage_Implementation() const override;

	/** 仅在服务器调用 */
	virtual void Die() override;
	
	/**
	 * 死亡时具体执行的操作
	 * 网络多播RPC:服务器发起调用，并广播到所有客户端执行
	 */
	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastHandleDeath();

	FORCEINLINE virtual bool IsDead_Implementation() const override { return bIsDead; }
	
	FORCEINLINE virtual const AActor* GetAvatar_Implementation() const override { return this; }

	FORCEINLINE virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() const override { return AttackMontages; }

	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TArray<FTaggedMontage> AttackMontages;
protected:
	virtual FVector GetWeaponSocketLocation_Implementation() override;;

	/** 死亡后溶解 */
	void Dissolve();

	/** Timeline控制角色Mesh溶解 */
	UFUNCTION(BlueprintImplementableEvent)
	void StartMeshDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);
	/** Timeline控制武器溶解 */
	UFUNCTION(BlueprintImplementableEvent)
	void StartWeaponDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Material")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Material")
	TObjectPtr<UMaterialInstance> WeaponMaterialInstance; 
	
private:
	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TObjectPtr<UAnimMontage> HitReactMontage;

	bool bIsDead = false;
#pragma endregion
	
#pragma region GAS
	/**
	 * MageCharacter类需要持久化 Attribute 数据，故采用 OwnerActor 和 AvatarActor 分离的方式：
	 * - MagePlayerState 为 OwnerActor，MageCharacterBase 为AvatarActor
	 * - MageEnemy类采用 OwnerActor 和 AvatarActor 合一的方式，OwnerActor, AvatarActor 都是自身
	 */

	/**
	 * OwnerActor 和 AvatarActor 都需要继承并实现 IAbilitySystemInterface
	 * - 我们在基类中继承了 IAbilitySystemInterface（子类 MageCharacter，MageEnemy也完成了继承）, 我们只需要再在 MagePlayerState 中继承即可。
	 * - 继承后需要实现GetAbilitySystemComponent()方法
	 */
	
public:
	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; };
	
	FORCEINLINE virtual UAttributeSet* GetAttributeSet() const { return AttributeSet; };
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	virtual void InitAbilityActorInfo();

	FORCEINLINE virtual int32 GetCharacterLevel() const override { return 0; }
	
	FORCEINLINE virtual ECharacterClass GetCharacterClass() const override { return ECharacterClass::None; }
	
protected:
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass,float Level) const;
	
	/** 使用GameplayEffect初始化默认属性, 仅可在服务器调用 */
	virtual void InitDefaultAttributes() const;
	
	/**
	 * 向ASC授予（Give）所有GameplayAbility，将 GA 的 Tag 添加到AbilitySpec，这些 Tag 将于输入的 Tag 进行匹配
	 *
	 * - 对于拥有 PlayerController 的 Character，在 PossessedBy() 中调用
	 */
	void AddCharacterAbilities() const;

private:
	UPROPERTY(EditAnywhere, Category = "Mage_GAS")
	TArray<TSubclassOf<UGameplayAbility>> CharacterAbilities;

	
#pragma endregion

#pragma region IGameplayTagAssetInterface
public:
	UFUNCTION(BlueprintCallable, Category = GameplayTags)
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTagContainer; }
	
private:
	UPROPERTY(EditAnywhere, Category = "Mage_GameplayTag")
	FGameplayTagContainer GameplayTagContainer;
#pragma endregion;
};
