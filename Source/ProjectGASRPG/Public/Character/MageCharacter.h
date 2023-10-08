// 

#pragma once

#include "CoreMinimal.h"
#include "MageCharacterBase.h"
#include "MageCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

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
	USpringArmComponent* GetSpringArm();
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mage_Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
#pragma endregion

#pragma region GAS
public:
	/* 服务器初始化 ASC/AS , */
	virtual void PossessedBy(AController* NewController) override;
	/* 客户端初始化 ASC/AS, 在 PlayerState 复制到客户端时进行回调*/
	virtual void OnRep_PlayerState() override;

private:
	void InitASCandAS();
#pragma endregion
};
