// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagePlayerController.generated.h"

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Maga_Input",meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMageInputConfig> MageInputConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAroundAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
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

	IEnemyInterface* LastActor;
	IEnemyInterface* CurrentActor;
	/* 鼠标射线检测选中物体并高亮 */
	void CursorTrace();

};
