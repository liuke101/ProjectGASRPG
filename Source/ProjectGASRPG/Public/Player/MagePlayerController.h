#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagePlayerController.generated.h"

class AMagicCircle;
class UNiagaraSystem;
class UDamageFloatingTextComponent;
class UWidgetComponent;
class USplineComponent;
class UMageAbilitySystemComponent;
struct FGameplayTag;
class UInputConfigDataAsset;
class IEnemyInterface;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTargetingActorChangedDelegate, AActor*/*NewTargetingActor*/, AActor*/* OldTargetingActor*/);

UCLASS()
class PROJECTGASRPG_API AMagePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMagePlayerController();

	virtual void PlayerTick(float DeltaTime) override;
	
protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

#pragma region InputActions
private:
	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UInputConfigDataAsset> InputConfigDataAsset;

	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UInputAction> LookAroundAction;
	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UInputAction> CameraZoomAction;

	/** Niagara点击特效 */
	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Input")
	TObjectPtr<UNiagaraSystem> ClickNiagaraSystem;

	/** 输入回调 */
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void LookAroundStart();
	void LookAroundEnd();
	void CameraZoom(const FInputActionValue& InputActionValue);
	
	/** 根据InputTag配置每个按键键对应的回调 */

	/** 默认InputTrigger:按下 */
	void AbilityInputTagStarted(FGameplayTag InputTag);
	/** 默认InputTrigger:不会触发X */
	void AbilityInputTagOngoing(FGameplayTag InputTag);
	/** 默认InputTrigger:持续 */
	void AbilityInputTagTriggered(FGameplayTag InputTag);
	/** 默认InputTrigger:不会触发X */
	void AbilityInputTagCanceled(FGameplayTag InputTag);
	/** 默认InputTrigger:释放 */
	void AbilityInputTagCompleted(FGameplayTag InputTag);
	
#pragma endregion

#pragma region GAS
private:
	UPROPERTY()
	TObjectPtr<UMageAbilitySystemComponent> MageAbilitySystemComponent;
	
public:
	UMageAbilitySystemComponent* GetMageASC();
	
#pragma endregion

#pragma region Targeting
public:
	UPROPERTY(BlueprintReadWrite,  Category = "MagePlayerController|Targeting")
	TArray<AActor*> TargetingActors;
	UPROPERTY(BlueprintReadWrite,  Category = "MagePlayerController|Targeting")
	TArray<AActor*> TargetingIgnoreActors;

	/** 最大切换目标数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagePlayerController|Targeting")
	int32 MaxSwitchTargetCount = 5;
	int32 SwitchTargetCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MagePlayerController|Targeting")
	int32 TargetingTime = 5;

	UFUNCTION(BlueprintPure, Category = "MagePlayerController|Targeting")
	AActor* GetTargetingActor() const;

	FOnTargetingActorChangedDelegate OnTargetingActorChanged;

	/** 技能范围指示 */
	UPROPERTY(EditDefaultsOnly, Category = "MagePlayerController|Targeting")
	TSubclassOf<AMagicCircle> MagicCircleClass;
private:
	UPROPERTY()
	AActor* LastTargetingActor;
	UPROPERTY()
	AActor* CurrentTargetingActor;
	UPROPERTY()
	FHitResult CursorHitResult;
	UPROPERTY()
	FTimerHandle TargetingTimerHandle;
	UPROPERTY()
	TObjectPtr<AMagicCircle> MagicCircle;
	
	/** 是否选中目标 */
	FORCEINLINE bool HasTargetingActor() const {return CurrentTargetingActor ? true : false;}
	
	/** 鼠标射线检测选中物体并高亮 */
	bool CursorHitTargeting();
	/** 切换目标 */
	void SwitchTargetingActor(AActor* NewTargetActor);
	/** Tab键 切换到离玩家最近的目标 */
	void SwitchCombatTarget();
	/** 取消选中目标 */
	void CancelTargetingActor();

	UFUNCTION()
	void TargetActorDeathCallback(AActor* DeadActor);

public:
	/** 技能范围指示 */
	UFUNCTION(BlueprintCallable)
	void ShowMagicCircle(UMaterialInstance* DecalMaterial = nullptr);
	UFUNCTION(BlueprintCallable)
	void HideMagicCircle();
	UFUNCTION(BlueprintCallable)
	FORCEINLINE FVector GetMagicCircleLocation() const {return MagicCircleLocation;}
private:
	FVector MagicCircleLocation = FVector::ZeroVector;
	void UpdateMagicCircleLocation();
#pragma endregion

#pragma region 寻路
public:
	/* bAutoRunning为true时，自动寻路 */
	void AutoRun();
private:
	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.0f; //按住的时间
	float ShortPressThreshold = 0.5f; //短按时间阈值
	bool bAutoRunning = false; //是否在寻路中,
	//bool bTargeting = false; //鼠标是否选中了物体
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage|Nav")
	float AutoRunAcceptanceRadius = 100.0f; //寻路接受半径，太小会导致无法停下
	
	UPROPERTY(EditDefaultsOnly, Category = "Mage|Nav")
	TObjectPtr<USplineComponent> SplineComponent;

	void SetCachedDestinationFromCursorHit();
#pragma endregion

#pragma region UI
public:
	/** 将 DamageFloatingText组件 附加到 TargetCharacter */
	UFUNCTION()
	void AttachDamageFloatingTextToTarget(float DamageValue, ACharacter* TargetCharacter, bool bIsCriticalHit);
private:
	UPROPERTY(EditDefaultsOnly, Category = "Mage|UI")
	TSubclassOf<UDamageFloatingTextComponent>  DamageFloatingTextComponentClass;
#pragma endregion

};
