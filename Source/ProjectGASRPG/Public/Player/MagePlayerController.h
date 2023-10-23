﻿// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagePlayerController.generated.h"

class UDamageFloatingTextComponent;
class UWidgetComponent;
class USplineComponent;
class UMageAbilitySystemComponent;
struct FGameplayTag;
class UMageInputConfig;
class IEnemyInterface;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

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
	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UMageInputConfig> MageInputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UInputAction> LookAroundAction;
	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UInputAction> CameraZoomAction;
	UPROPERTY(EditDefaultsOnly, Category = "Maga_Input")
	TObjectPtr<UInputAction> CtrlAction;

	/* 输入回调 */
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void LookAroundStart();
	void LookAroundEnd();
	void CameraZoom(const FInputActionValue& InputActionValue);
	
	FORCEINLINE void CtrlPressed() { bCtrlPressed = true; }
	FORCEINLINE void CtrlReleased() { bCtrlPressed = false; }
	bool bCtrlPressed = false;
	
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagHold(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	
#pragma endregion

#pragma region GAS
private:
	UPROPERTY()
	TObjectPtr<UMageAbilitySystemComponent> MageAbilitySystemComponent;
	
public:
	UMageAbilitySystemComponent* GetAbilitySystemComponent();
#pragma endregion

#pragma region 物品交互
private:
	IEnemyInterface* LastActor = nullptr;
	IEnemyInterface* CurrentActor = nullptr;
	FHitResult CursorHitResult;
	/* 鼠标射线检测选中物体并高亮 */
	void CursorTrace();
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
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Input")
	float AutoRunAcceptanceRadius = 100.0f; //寻路接受半径，太小会导致无法停下
	UPROPERTY(EditDefaultsOnly, Category = "Mage_Input")
	TObjectPtr<USplineComponent> SplineComponent;

	bool bTargeting(); //鼠标是否选中了物体
	void SetCachedDestinationFromCursorHit();
	
#pragma endregion

#pragma region UI
public:
	/** 将 DamageFloatingText组件 附加到 TargetCharacter */
	UFUNCTION(Client, Reliable)
	void AttachDamageFloatingTextToTarget(float DamageValue, ACharacter* TargetCharacter);
private:
	UPROPERTY(EditDefaultsOnly, Category = "Mage_UI")
	TSubclassOf<UDamageFloatingTextComponent>  DamageFloatingTextComponentClass;
#pragma endregion
};
