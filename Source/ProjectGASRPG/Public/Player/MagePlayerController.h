// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagePlayerController.generated.h"

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

private:
#pragma region InputActions
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

	/* 输入回调 */
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void LookAroundStart();
	void LookAroundEnd();
	void CameraZoom(const FInputActionValue& InputActionValue);

	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHold(FGameplayTag InputTag);
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
	IEnemyInterface* LastActor;
	IEnemyInterface* CurrentActor;
	/* 鼠标射线检测选中物体并高亮 */
	void CursorTrace();
#pragma endregion
};
