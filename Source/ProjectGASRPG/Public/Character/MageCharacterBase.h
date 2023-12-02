#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GAS/Data/CharacterClassDataAsset.h"
#include "Interface/CombatInterface.h"
#include "MageCharacterBase.generated.h"

class UDebuffNiagaraComponent;
class UNiagaraSystem;
class UGameplayTagsComponent;
struct FGameplayTagContainer;
class UGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UAbilitySystemComponent;

/** 角色抽象基类，不能实例化 */
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> Weapon;
	
	/** 武器附加到Mesh的Socket(蓝图构造函数中进行Attach) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageCharacter|Weapon")
	FName WeaponAttachSocket;
#pragma endregion

#pragma region ICombatInterface
public:
	FORCEINLINE virtual int32 GetCharacterLevel() const override { return 0; }
	
	FORCEINLINE virtual ECharacterClass GetCharacterClass() const override { return ECharacterClass::None; }

	FORCEINLINE virtual USkeletalMeshComponent* GetWeapon_Implementation() override {return Weapon;}
	
	/** 基于GameplayTag返回Socket位置, 支持武器、双手等 */
	virtual FVector GetWeaponSocketLocationByTag_Implementation(const FGameplayTag& SocketTag) const override;
	
	FORCEINLINE virtual UAnimMontage* GetHitReactMontage_Implementation() const override {return HitReactMontage;}

	virtual void Die(const FVector& DeathImpulse) override;
	
	FORCEINLINE virtual bool IsDead_Implementation() const override { return bIsDead; }
	
	FORCEINLINE virtual const AActor* GetAvatar_Implementation() const override { return this; }

	FORCEINLINE virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() const override { return AttackMontages; }

	FORCEINLINE virtual UNiagaraSystem* GetHitEffect_Implementation() const override {return HitEffect;}
	
	// 从FTaggedMontage数组中随机获取一个对象
	UFUNCTION(BlueprintPure)
	virtual FTaggedMontage GetRandomAttackMontage_Implementation() const override;

	virtual FTaggedMontage GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag) const override;
	
	UPROPERTY(EditAnywhere, Category = "MageCharacter|CombatInterface")
	TArray<FTaggedMontage> AttackMontages;

	/**
	 * InitAbilityActorInfo()中当注册ASC完成时广播,通知 DebuffNiagaraComponent 此时Character已经拥有了ASC
	 * - 使用接口实现 Getter 委托, 这样组件就不需要持有Character的引用, 防止循环依赖（解耦）
	 */
	FOnAscRegisteredDelegate OnASCRegisteredDelegate;
	FORCEINLINE virtual FOnAscRegisteredDelegate& GetOnASCRegisteredDelegate() override { return OnASCRegisteredDelegate; }

	/**
	 * 死亡时广播
	 * - 通知 DebuffNiagaraComponent 此时Character已经死亡,停止激活 Niagara
	 * - 通知 BeamGA 死亡时停止所有 GC
	 */
	FOnDeathDelegate OnDeathDelegate;
	FORCEINLINE virtual FOnDeathDelegate& GetOnDeathDelegate() override { return OnDeathDelegate; }

	/** 眩晕状态 */
	UPROPERTY(BlueprintReadOnly)
	bool bIsStun = false;
	virtual void StunTagChanged(const FGameplayTag CallbackTag, const int32 NewCount);
protected:
	/** 根据攻击蒙太奇对应的Tag ——> 武器产生攻击判定的Soceket(例如武器顶端，双手等) */
	UPROPERTY(EditAnywhere, Category = "MageCharacter|CombatInterface")
	TMap<FGameplayTag,FName> AttackSocketTag_To_WeaponSocket;

private:
	UPROPERTY(EditAnywhere, Category = "MageCharacter|CombatInterface")
	TObjectPtr<UAnimMontage> HitReactMontage;

	bool bIsDead = false;

	UPROPERTY(EditAnywhere, Category = "MageCharacter|CombatInterface")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditAnywhere, Category = "MageCharacter|CombatInterface")
	TObjectPtr<USoundBase> DeathSound;

#pragma endregion
	
#pragma region GAS
	/**
	 * (1) MageCharacter类需要持久化 Attribute 数据，故采用 OwnerActor 和 AvatarActor 分离的方式：
	 *	- MagePlayerState 为 OwnerActor，MageCharacter 为AvatarActor
	 * (2)MageEnemy类采用 OwnerActor 和 AvatarActor 合一的方式，OwnerActor, AvatarActor 都是自身
	 */

	/**
	 * OwnerActor 和 AvatarActor 都需要继承并实现 IAbilitySystemInterface
	 * - 我们在基类中继承了 IAbilitySystemInterface（子类 MageCharacter，MageEnemy也完成了继承）, 我们只需要再在 MagePlayerState 中继承即可
	 * - 继承后需要实现 GetAbilitySystemComponent() 方法
	 */
	
public:
	/** 初始化ASC */
	virtual void InitASC();

	FORCEINLINE virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; };
	FORCEINLINE virtual UAttributeSet* GetAttributeSet() const { return AttributeSet; };
	
	FORCEINLINE virtual int32 GetSummonCount_Implementation() const override { return SummonCount; }
	FORCEINLINE virtual void ModifySummonCount_Implementation(const int32 Count) override { SummonCount += Count; }
protected:
	/** 使用GameplayEffect初始化默认属性, 仅可在服务器调用 */
	virtual void InitDefaultAttributes() const;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "MageCharacter|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "MageCharacter|GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
private:
	/** 召唤物数量 */
	UPROPERTY(EditAnywhere, Category = "MageCharacter|GAS")
	int32 SummonCount = 0;
#pragma endregion

#pragma region IGameplayTagAssetInterface
public:
	UFUNCTION(BlueprintCallable, Category = GameplayTags)
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTagContainer; }
	
private:
	// 存储角色拥有的GameplayTag
	UPROPERTY(EditAnywhere, Category = "MageCharacter|GameplayTag")
	FGameplayTagContainer GameplayTagContainer;
#pragma endregion;

#pragma region Misc
protected:
	float DefaultMaxWalkSpeed = 600.f;
	
	//将所有骨骼网格体组件收集到数组，用于后续的溶解、高亮描边等效果
	void CollectMeshComponents();
	
	UPROPERTY()
	TArray<USceneComponent*> MeshComponents;

	/** 死亡后溶解 */
	void Dissolve();

	/** Timeline控制角色Mesh溶解 */
	UFUNCTION(BlueprintImplementableEvent)
	void StartMeshDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MageCharacter|Misc|Material")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,Category = "MageCharacter|Misc|VFX")
	TObjectPtr<UDebuffNiagaraComponent> BurnDebuffNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,Category = "MageCharacter|Misc|VFX")
	TObjectPtr<UDebuffNiagaraComponent> FrozenDebuffNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,Category = "MageCharacter|Misc|VFX")
	TObjectPtr<UDebuffNiagaraComponent> StunDebuffNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite,Category = "MageCharacter|Misc|VFX")
	TObjectPtr<UDebuffNiagaraComponent> BleedDebuffNiagara;
#pragma endregion
};


