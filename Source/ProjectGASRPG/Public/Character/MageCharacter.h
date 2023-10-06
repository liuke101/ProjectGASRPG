// 

#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "MageCharacter.generated.h"

UCLASS()
class PROJECTGASRPG_API AMageCharacter : public AMageCharacterBase
{
	GENERATED_BODY()

public:
	AMageCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region Camera
public:
	void SetCameraDistance(float Value);
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Camera", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
#pragma endregion
};
