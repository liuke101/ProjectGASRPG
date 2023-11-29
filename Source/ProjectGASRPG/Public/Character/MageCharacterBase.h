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

UCLASS(Abstract)
class PROJECTGASRPG_API AMageCharacterBase : public ACharacter, public IAbilitySystemInterface,public IGameplayTagAssetInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	AMageCharacterBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;

#pragma region Weapon
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> Weapon;

	/** 武器附加到Mesh的Socket(蓝图构造函数中进行Attach) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Weapon")
	FName WeaponAttachSocket;
	
#pragma endregion

#pragma region ICombatInterface
public:
	virtual UAnimMontage* GetHitReactMontage_Implementation() const override;

	/** 仅在服务器调用 */
	virtual void Die(const FVector& DeathImpulse) override;
	
	/**
	 * 死亡时具体执行的操作
	 * 网络多播RPC:服务器发起调用，并广播到所有客户端执行
	 */
	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastHandleDeath(const FVector& DeathImpulse);

	FORCEINLINE virtual bool IsDead_Implementation() const override { return bIsDead; }
	
	FORCEINLINE virtual const AActor* GetAvatar_Implementation() const override { return this; }

	FORCEINLINE virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() const override { return AttackMontages; }

	FORCEINLINE virtual UNiagaraSystem* GetHitEffect_Implementation() const override {return HitEffect;}
	
	// 从FTaggedMontage数组中随机获取一个对象
	UFUNCTION(BlueprintPure)
	virtual FTaggedMontage GetRandomAttackMontage_Implementation() const override;

	virtual FTaggedMontage GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag) const override;
	
	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TArray<FTaggedMontage> AttackMontages;

	/**
	 * InitAbilityActorInfo()中当注册ASC完成时广播,通知DebuffNiagaraComponent此时Character已经拥有了ASC
	 * - 使用接口实现 Getter 委托, 这样组件就不需要持有Character的引用, 防止循环依赖（解耦）
	 */
	FOnAscRegisteredDelegate OnASCRegisteredDelegate;
	FORCEINLINE virtual FOnAscRegisteredDelegate& GetOnASCRegisteredDelegate() override { return OnASCRegisteredDelegate; }

	/**
	 * 死亡时广播
	 * - 通知DebuffNiagaraComponent此时Character已经死亡,停止激活Niagara
	 * - 通知 BeamGA 死亡时停止所有 GC
	 */
	FOnDeathDelegate OnDeathDelegate;
	FORCEINLINE virtual FOnDeathDelegate& GetOnDeathDelegate() override { return OnDeathDelegate; }

	/** 眩晕状态 */
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsStun = false;
	virtual void StunTagChanged(const FGameplayTag CallbackTag, const int32 NewCount);
protected:
	FORCEINLINE virtual USkeletalMeshComponent* GetWeapon_Implementation() override {return Weapon;}
	
	/** 基于GameplayTag返回Socket位置, 支持武器、双手等 */
	virtual FVector GetWeaponSocketLocationByTag_Implementation(const FGameplayTag& SocketTag) const override;

	/** 根据攻击蒙太奇对应的Tag ——> 武器产生攻击判定的Soceket(例如武器顶端，双手等) */
	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TMap<FGameplayTag,FName> AttackSocketTag_To_AttackTriggerSocket;

	
private:
	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TObjectPtr<UAnimMontage> HitReactMontage;

	bool bIsDead = false;

	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditAnywhere, Category = "Mage_CombatInterface")
	TObjectPtr<USoundBase> DeathSound;

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

	FORCEINLINE virtual int32 GetSummonCount_Implementation() const override { return SummonCount; }

	FORCEINLINE virtual void ModifySummonCount_Implementation(const int32 Count) override { SummonCount += Count; }
protected:
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass,float Level) const;
	
	/** 使用GameplayEffect初始化默认属性, 仅可在服务器调用 */
	virtual void InitDefaultAttributes() const;

private:
	/** 召唤物数量 */
	UPROPERTY(EditAnywhere, Category = "Mage_GAS")
	int32 SummonCount = 0;
#pragma endregion

#pragma region IGameplayTagAssetInterface
public:
	UFUNCTION(BlueprintCallable, Category = GameplayTags)
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTagContainer; }
	
private:
	UPROPERTY(EditAnywhere, Category = "Mage_GameplayTag")
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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mage_Misc|Material")
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UDebuffNiagaraComponent> BurnDebuffNiagara;

#pragma endregion
};


