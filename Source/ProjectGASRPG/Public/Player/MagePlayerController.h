// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MagePlayerController.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAroundAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maga_Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> CameraZoomAction;
	
	void Move(const FInputActionValue& InputActionValue);
	void Look(const FInputActionValue& InputActionValue);
	void LookAroundStart();
	void LookAroundEnd();
	void CameraZoom(const FInputActionValue& InputActionValue);
	
#pragma endregion

	IEnemyInterface* LastActor;
	IEnemyInterface* CurrentActor;
	void CursorTrace();

};
