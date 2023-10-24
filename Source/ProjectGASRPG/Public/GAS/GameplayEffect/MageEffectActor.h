#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "MageEffectActor.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USphereComponent;

UENUM(BlueprintType)
enum class EffectApplicationPolicy : uint8
{
	EAP_ApplyOnBeginOverlap UMETA(DisplayName = "ApplyOnBeginOverlap"),
	EAP_ApplyOnEndOverlap UMETA(DisplayName = "ApplyOnEndOverlap"),
	EAP_DoNotApply UMETA(DisplayName = "DoNotApply")
};

UENUM(BlueprintType)
enum class EffectRemovalPolicy : uint8
{
	ERP_RemoveOnEndOverlap UMETA(DisplayName = "RemoveOnEndOverlap"),
	ERP_DoNotRemove UMETA(DisplayName = "DoNotRemove")
};

UCLASS()
class PROJECTGASRPG_API AMageEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AMageEffectActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Mage_Effects")
	void ApplyEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	UFUNCTION(BlueprintCallable, Category = "Mage_Effects")
	void OnEffectBeginOverlap(AActor* TargetActor);
	UFUNCTION(BlueprintCallable, Category = "Mage_Effects")
	void OnEffectEndOverlap(AActor* TargetActor);

#pragma region "Duration Type"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Instant")
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Instant")
	EffectApplicationPolicy InstantEffectApplicationPolicy = EffectApplicationPolicy::EAP_DoNotApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Duration")
	TSubclassOf<UGameplayEffect> DurationGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Duration")
	EffectApplicationPolicy DurationEffectApplicationPolicy = EffectApplicationPolicy::EAP_DoNotApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Infinite")
	TSubclassOf<UGameplayEffect> InfiniteGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Infinite")
	EffectApplicationPolicy InfiniteEffectApplicationPolicy = EffectApplicationPolicy::EAP_DoNotApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects|Infinite")
	EffectRemovalPolicy InfiniteEffectRemovalPolicy = EffectRemovalPolicy::ERP_RemoveOnEndOverlap;
#pragma endregion
	
	TMap<FActiveGameplayEffectHandle,UAbilitySystemComponent*> ActiveEffectHandles;
	
	/** Effect等级，配合CurveTable进行配置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Effects")
	float EffectLevel = 1.f;
	
};
