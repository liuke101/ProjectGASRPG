﻿#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "Interface/PlayerInterface.h"
#include "MageCharacter.generated.h"

class UNiagaraComponent;
class AMagePlayerState;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class PROJECTGASRPG_API AMageCharacter : public AMageCharacterBase, public IPlayerInterface
{
	GENERATED_BODY()

public:
	AMageCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual AMagePlayerState* GetMagePlayerState() const;
	
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
	 * - 对于拥有 PlayerController 的 Character，在 PossessedBy() 中调用(仅服务器执行)
	 */
	void GiveCharacterAbilities() const;

private:
	/** 主动技能 */
	UPROPERTY(EditAnywhere, Category = "Mage_GAS")
	TArray<TSubclassOf<UGameplayAbility>> CharacterAbilities;

	/** 被动技能 */
	UPROPERTY(EditAnywhere, Category = "Mage_GAS")
	TArray<TSubclassOf<UGameplayAbility>> PassiveAbilities;
	
	/** 初始化 */
	virtual void InitAbilityActorInfo() override;

	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultVitalAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttribute;
	UPROPERTY(EditDefaultsOnly, Category="Mage_GAS")
	TSubclassOf<UGameplayEffect> DefaultResistanceAttribute;
	
#pragma endregion


#pragma region PlayerInterface
public:
	virtual void LevelUp() override;
	
	virtual int32 GetExp() const override;
	virtual void AddToExp(const int32 InExp) override;
	
	/** 查询LevelDataAsset, 根据经验值获取等级数 */
	virtual int32 FindLevelForExp(const int32 InExp) const override;
	virtual void AddToLevel(const int32 InLevel) override;

	virtual int32 GetAttributePointReward(const int32 Level) const override;
	virtual void AddToAttributePoint(const int32 InPoints) override;
	virtual int32 GetAttributePoint() const override;

	virtual int32 GetSkillPointReward(const int32 Level) const override;
	virtual void AddToSkillPoint(const int32 InPoints) override;
	virtual int32 GetSkillPoint() const override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mage_Misc|VFX")
	TObjectPtr<UNiagaraComponent> LevelUpNiagara;
private:
	/** 多播升级特效, 让所有客户端看到 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLevelUpEffect() const;
	
#pragma endregion
	
	
#pragma region CombatInterface
public:
	virtual  int32 GetCharacterLevel() const override;
	virtual ECharacterClass GetCharacterClass() const override;
#pragma endregion

#pragma region Misc
protected:
	
#pragma endregion

#pragma region Animation
public:
	UPROPERTY(BlueprintReadWrite,Replicated=true, Category="Mage_GA|Animation")
	bool bIsCastingLoop = false;

	FORCEINLINE virtual  void SetInCastingLoop_Implementation(const bool bInIsCastingLoop) override {bIsCastingLoop = bInIsCastingLoop;}
#pragma endregion
};
