#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MageGameplayAbility.generated.h"


UCLASS()
class PROJECTGASRPG_API UMageGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UMageGameplayAbility();

public:
	UPROPERTY(EditAnywhere, Category = "Mage_GA|Base")
	int32 StartupAbilityLevel = 1; //技能初始等级(默认为1即可)
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage_GA|Base")
	FGameplayTag StartupInputTag;

	/** 用于SkillTreeMenu获取技能描述, 子类可以重载它 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|Base")
	FString GetDescription(const int32 AbilityLevel);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|Base")
	FString GetNextLevelDescription(const int32 AbilityLevel);
	static FString GetLockedDescription(const int32 LevelRequirement);

	/** 技能花费 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Mage_GA|Cost")
	FScalableFloat HealthCost;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Mage_GA|Cost")
	FScalableFloat ManaCost;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Mage_GA|Cost")
	FScalableFloat VitalityCost;

	/** 技能冷却，每个技能需要单独设置CooldownTag */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Mage_GA|Cooldown")
	FScalableFloat CooldownDuration;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Mage_GA|Cooldown")
	FGameplayTagContainer CooldownTags;

	// 临时容器，我们将在 GetCooldownTags() 中返回其指针。这将是我们的 CooldownTags 和 Cooldown GE's Cooldown Tags 的冷却标签的组合。
	UPROPERTY()
	FGameplayTagContainer TempCooldownTags;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	/** 重载, 实现Cooldown GE复用 */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	float GetCost(const FGameplayAttribute& Attribute, const int32 AbilityLevel) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|Cost")
	float GetHealthCost(const int32 AbilityLevel) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|Cost")
	float GetManaCost(const int32 AbilityLevel) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|Cost")
	float GetVitalityCost(const int32 AbilityLevel) const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mage_GA|Cooldown")
	float GetCooldown(const int32 AbilityLevel) const;
};
